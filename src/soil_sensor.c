#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

#include "soil_sensor.h"
static int err;

LOG_MODULE_DECLARE(Plant_sensor);

#define SAMPLE_SIZE 6
// static int lowest_mv = 2000; // set too high to begin with
static int wet_tolerance = 0;
static int dry_tolerance = 0;
static uint64_t start_time_calibrate_threshold;
static int dry_plant_threshold = 0;
static int wet_plant_threshold = 0;
bool soil_moisture_calibrated = false;
static int samples[SAMPLE_SIZE] = {0};
static int * ptr_sample = samples;
static int * ptr_end_sample = samples + SAMPLE_SIZE;
static bool get_time = true;
// adc
static int16_t buf;
static struct adc_sequence sequence = {
    .buffer = &buf,
    /* buffer size in bytes, not number of samples */
    .buffer_size = sizeof(buf),
    // Optional
    //.calibrate = true,
};

int moisture_val_mv = 0;
// initialize sensor
static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));

void initialize_adc()
{
    int err;
    // start_time_calibrate_threshold = k_uptime_get();
    // ADC
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
        // continue;
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
    }
}

void calibrate_soil_sensor()
{
    // 1. dry condition first
    // 1.2 collect moisture_mv
    read_soil_moisture_mv();
    if (get_time)
    {
        start_time_calibrate_threshold = k_uptime_get();
        get_time = false;
    }
    
    
    if (ptr_sample < ptr_end_sample)
    {
        *ptr_sample = moisture_val_mv;
        ptr_sample++;
    }
    int64_t elapsed_ms = k_uptime_get() - start_time_calibrate_threshold;
    if (elapsed_ms >= 10000)
    {
        LOG_INF("sampling done. Now printing result.\n");
        LOG_INF("[");
        ptr_sample = samples;
        for (int i = 0; ptr_sample < ptr_end_sample; ++i)
        {
            LOG_INF("%d mV", samples[i]);
            ptr_sample++;
        }
        LOG_INF("]");
        soil_moisture_calibrated = true;
    }
    // 1.3 get max value
    // 1.4 get dry tolerance
    // 2. start pump for 10 sec.
    // 2.1 wait for 10 sec. (wait for sensor at bottom to detect water)
    // 3.1 start pump again.. repeat 
    // 4. water detected at bottom
    // 5. collect moisture_mv
    // 5.1 get min value
    // 5.2 get wet tolerance

    // find out what range to use where the value does not change,
    // as the measured voltage will change a bit even if water level stays the same...
    // read_soil_moisture_mv();
    // if (dry_plant_threshold == 0)
    // {
    //     dry_plant_threshold = moisture_val_mv;
    // }
    // if (moisture_val_mv < lowest_mv)
    // {
    //     lowest_mv = moisture_val_mv;
    // }
    // int64_t elapsed_ms = k_uptime_get() - start_time_calibrate_threshold;
    // if (elapsed_ms >= 40000)
    // {
    //     wet_plant_threshold = lowest_mv;
    //     LOG_INF(" wet threshold set: %d", wet_plant_threshold);
    //     soil_moisture_calibrated = true;
    // }
}

int mv_to_percentage(int value)
{
    // scale: 0% -> dry, 100% -> wet
    // clamp value within input range
    int wet_value_min = wet_plant_threshold - wet_tolerance;
    int wet_value_max = wet_plant_threshold + wet_tolerance;

    int dry_value_min = dry_plant_threshold - dry_tolerance;
    int dry_value_max = dry_plant_threshold + dry_tolerance;

    int percentage;

    if (wet_value_min <= value && wet_value_max >= value)
    {
        return 100;
    }
    else if (dry_value_min <= value && dry_value_max >= value)
    {
        return 0;
    }
    else if (wet_value_max < value && dry_value_min > value)
    {
        // map the value
        int numerator = value - wet_value_max;
        int denominator = dry_value_min - wet_value_max;
        percentage = 100 * (denominator - numerator) / denominator;

        return percentage;
    }
    // handle edge cases and map values outside the min/max
    else if (value < wet_value_min)
    {
        return 100;
    }
    else if (value > dry_value_max)
    {
        return 0;
    }
    
    return -1;
}

int get_tolerance(int max, int min)
{
    // standard deviation
    // get mean, get standard deviation, tolerance = 1 x std dev or 2 x std dev
    // half the range
    return (max - min) / 2;
}