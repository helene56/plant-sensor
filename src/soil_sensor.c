#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <math.h>
#include <stdlib.h>

#include "soil_sensor.h"
#include "sensor_config.h"
static int err;

LOG_MODULE_DECLARE(Plant_sensor);

#define SAMPLE_SIZE 200
#define STABLE_SAMPLE_SIZE 99
#define MOISTURE_READ_SIZE 100
#define MAX_TOLERANCE 15
#define MINUTE_WAIT_TIME 1 // TODO: TEMP VALUE, should be initialized by central over BLE
#define IDEAL_WAIT_TIME_MIN K_MINUTES(MINUTE_WAIT_TIME)
// tolerances
static int wet_tolerance = 0;
static int dry_tolerance = 0;
// thresholds
static int dry_plant_threshold = 0;
static int wet_plant_threshold = 0;
static int ideal_plant_threshold = 0;
// states
bool soil_moisture_calibrated = false;
// enum SOIL_SENSOR_STATE CURRENT_SOIL_STATE = DRY;
// sampling
static int samples[SAMPLE_SIZE] = {0};
static int *ptr_sample = samples;
static int *ptr_end_sample = samples + SAMPLE_SIZE;
static int *ptr_end_read_moisture = samples + MOISTURE_READ_SIZE;
// low pass filter
static signed long smooth_data_int;
static signed long smooth_data_fp;
const int beta = 4;
int raw_data = 0;
const int fp_shift = 8;

// adc
static int16_t buf;
static struct adc_sequence sequence = {
    .buffer = &buf,
    /* buffer size in bytes, not number of samples */
    .buffer_size = sizeof(buf),
    // Optional
    // .calibrate = true,
};

int moisture_val_mv = 0;
// initialize sensor
static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));

void initialize_adc()
{
    int err;

    if (!adc_is_ready_dt(&adc_channel))
    {
        LOG_ERR("ADC controller devivce %s not ready", adc_channel.dev->name);
        return;
    }

    err = adc_channel_setup_dt(&adc_channel);
    if (err < 0)
    {
        LOG_ERR("Could not setup channel #%d (%d)", 0, err);
        return;
    }

    err = adc_sequence_init_dt(&adc_channel, &sequence);
    if (err < 0)
    {
        LOG_ERR("Could not initalize sequnce");
        return;
    }
}

void read_soil_moisture_mv()
{

    err = adc_read(adc_channel.dev, &sequence);
    if (err < 0)
    {
        LOG_ERR("Could not read (%d)", err);
        return;
    }

    moisture_val_mv = (int)buf;

    err = adc_raw_to_millivolts_dt(&adc_channel, &moisture_val_mv);
    /* conversion to mV may not be supported, skip if not */
    if (err < 0)
    {
        LOG_WRN(" (value in mV not available)\n");
    }
    else
    {
        // LOG_INF(" = %d mV", moisture_val_mv);
        // smooth_data();
    }
}

int smooth_data()
{
    raw_data = moisture_val_mv;
    raw_data <<= fp_shift;
    smooth_data_fp = (smooth_data_fp << beta) - smooth_data_fp;
    smooth_data_fp += raw_data;
    smooth_data_fp >>= beta;
    smooth_data_int = smooth_data_fp >> fp_shift;
    // LOG_INF("measured data: %d mV", moisture_val_mv);
    // LOG_INF("smooth data: %ld mV", smooth_data_int);
    return smooth_data_int;
}

void read_smooth_soil()
{
    read_soil_moisture_mv();

    if (ptr_sample < ptr_end_sample)
    {
        *ptr_sample = smooth_data();
        ptr_sample++;
    }

    if (ptr_sample >= ptr_end_read_moisture)
    {
        ptr_sample--;
        smooth_soil_val = *ptr_sample;
        // reset
        ptr_sample = samples;
    }
}

// following operations are done:
// 1. read current soil moisture
// 2. get smoothed soil moisture and insert into samples
// 3. when samples is filled -> dry threshold and tolerance is set
// 4. pump is activated and soil is wet
// 5. restarting sampling
// 6. when samples is filled -> wet threshold and tolerance is set
// 7. wait time
// 8. when wait time is done -> latest smooth value is set as ideal threshold
// 9. calibration done
void calibrate_soil_sensor(CalibrationContext *ctx)
{
    read_soil_moisture_mv();

    if (ptr_sample < ptr_end_sample)
    {
        *ptr_sample = smooth_data();
        ptr_sample++;
    }

    if (ptr_sample >= ptr_end_sample)
    {
        if (ctx->current_soil_state != IDEAL)
        {
            // scale: 0% -> dry, 100% -> wet
            int min_val = samples[STABLE_SAMPLE_SIZE];
            int max_val = samples[STABLE_SAMPLE_SIZE];

            ptr_sample = samples;
            ptr_sample += STABLE_SAMPLE_SIZE;

            for (; ptr_sample < ptr_end_sample; ++ptr_sample)
            {
                int val = *ptr_sample;
                if (val > max_val)
                {
                    max_val = val;
                }
                else if (val < min_val)
                {
                    min_val = val;
                }
            }
            int tolerance = max_val - min_val;
            // redo sampling as samples are not stable enough
            if (tolerance >= MAX_TOLERANCE)
            {
                ptr_sample = samples;
                LOG_INF("redoing sampling, tolerance: %d is too large", tolerance);
            }
            else
            {
                // temp setup to calibrate sensor
                if (ctx->current_soil_state == DRY)
                {
                    dry_tolerance = max_val - min_val;
                    dry_plant_threshold = max_val;
                    LOG_INF("dry tolerance: %d, dry threshold: %d", dry_tolerance, dry_plant_threshold);
                    // restart
                    ptr_sample = samples;
                    // LOG_INF("pumping water.."); // elsewhere a thread should start pumping/manage water
                    // CURRENT_SOIL_STATE = WET;
                    // k_sleep(K_MSEC(60000 * 5)); // delay for 5 min. for sensor to acclimate to water
                    ctx->soil_moisture_calibrated = true; // TODO: rename so it is clear this is calibration for dry state
                }
                else if (ctx->current_soil_state == WET)
                {
                    wet_tolerance = max_val - min_val;
                    wet_plant_threshold = min_val;
                    LOG_INF("wet tolerance: %d, wet threshold: %d", wet_tolerance, wet_plant_threshold);
                    // restart
                    ctx->soil_moisture_calibrated = true;
                    ptr_sample = samples;
                    LOG_INF("Now waiting for %d min., to stabilize soil.", MINUTE_WAIT_TIME);
                    // CURRENT_SOIL_STATE = IDEAL;
                    k_sleep(IDEAL_WAIT_TIME_MIN);
                }
            }
        }
        else if (ctx->current_soil_state == IDEAL)
        {
            // get the latest value
            ideal_plant_threshold = samples[SAMPLE_SIZE - 1];
            LOG_INF("Ideal threshold: %d", ideal_plant_threshold);
            // sensor calibration is done
            ctx->soil_moisture_calibrated = true;
        }
    }
}

int mv_to_percentage(int value)
{
    // scale: 0% -> dry, 100% -> wet
    // clamp value within input range
    // TODO: is tolerance even necessary??
    // maybe an idea is to redo calibration if tolerance is too high? too much diff between max/min
    // int wet_value_min = wet_plant_threshold - wet_tolerance;
    // int dry_value_max = dry_plant_threshold + dry_tolerance;
    int wet_value_min = wet_plant_threshold;
    int dry_value_max = dry_plant_threshold;
    // TODO: add check that wet value and dry value !=
    if (value <= wet_value_min)
    {
        return 100;
    }
    else if (value >= dry_value_max)
    {
        return 0;
    }
    else
    {
        int numerator = dry_value_max - value;
        int denominator = dry_value_max - wet_value_min;
        int percentage = ((numerator * 100) / denominator);

        if (percentage < 0)
            percentage = 0;
        else if (percentage > 100)
            percentage = 100;

        return percentage;
    }
}
