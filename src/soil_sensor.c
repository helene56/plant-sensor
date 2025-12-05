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
#define MOISTURE_READ_SIZE 5
#define MAX_TOLERANCE 15
#define MINUTE_WAIT_TIME 120   // TODO: TEMP VALUE, should be initialized by central over BLE
#define SECONDS_WAIT_TIME 10 // TODO: temp value, only for testing
#define IDEAL_WAIT_TIME_SEC K_SECONDS(SECONDS_WAIT_TIME)
#define IDEAL_WAIT_TIME_MIN K_MINUTES(MINUTE_WAIT_TIME)
// thresholds - should be first defined in sensor_config
// static int dry_plant_threshold = 0;
// static int wet_plant_threshold = 0;
// static int ideal_plant_threshold = 0;

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
    LOG_INF("smooth data: %ld mV", smooth_data_int);
    return smooth_data_int;
}

int read_smooth_soil()
{
    // read the raw value
    read_soil_moisture_mv();
    smooth_soil_val = smooth_data();
    return check_stability(smooth_soil_val);
}

void calibrate_soil_sensor(CalibrationContext *ctx, struct nvs_fs *fs)
{

    int soil_reading = read_smooth_soil();

    if (soil_reading)
    {
        if (ctx->current_soil_state != IDEAL)
        {

            if (ctx->current_soil_state == DRY)
            {
                dry_plant_threshold = smooth_soil_val;
                // write to flash to save value
                (void)nvs_write(
                fs, DRY_PLANT_ID, &dry_plant_threshold,
                sizeof(dry_plant_threshold));
                LOG_INF("dry stable reading achieved at %d.", dry_plant_threshold);
                ctx->soil_moisture_ready_to_calibrate = true; // TODO: rename so it is clear this is calibration for dry state
            }
            else if (ctx->current_soil_state == WET)
            {
                wet_plant_threshold = smooth_soil_val;
                (void)nvs_write(
                fs, WET_PLANT_ID, &wet_plant_threshold,
                sizeof(wet_plant_threshold));
                LOG_INF("wet stable reading achieved at %d.", wet_plant_threshold);
                // restart
                ctx->soil_moisture_ready_to_calibrate = true;
                LOG_INF("Now waiting for %d min., to stabilize soil.", MINUTE_WAIT_TIME);
                k_sleep(IDEAL_WAIT_TIME_MIN);
            }
        }
        else if (ctx->current_soil_state == IDEAL)
        {

            ideal_plant_threshold = smooth_soil_val;
            (void)nvs_write(
                fs, IDEAL_PLANT_ID, &ideal_plant_threshold,
                sizeof(ideal_plant_threshold));
            LOG_INF("ideal stable reading achieved at %d.", ideal_plant_threshold);
            // sensor calibration is done
            ctx->soil_moisture_ready_to_calibrate = true;
        }
    }
}

int mv_to_percentage(int value)
{
    // scale: 0% -> dry, 100% -> wet
    // clamp value within input range
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

int check_stability(int current_val)
{
    int tolerance = 3;
    int stable_count = 5;
    static int previous_val = 0;
    static int count = 0;
    if (abs(previous_val - current_val) > tolerance)
    {
        // outside tolerance, current_val not stable enough
        // reset count
        count = 0;
    }
    else
    {
        count++;
    }
    LOG_INF("count for stability: %d", count);
    previous_val = current_val;

    if (count >= stable_count)
    {
        // current val is stable enough
        LOG_INF("stable smooth value");
        // reset
        count = 0;
        previous_val = 0;
        return 1;
    }
    else
    {
        return 0;
    }
}
