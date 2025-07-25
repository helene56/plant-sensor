#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

#include "soil_sensor.h"
static int err;

LOG_MODULE_DECLARE(Plant_sensor);

static int lowest_mv = 2000;
static uint64_t start_time_calibrate_threshold;
static int dry_plant_threshold = 0;
static int wet_plant_threshold = 0;
bool soil_moisture_calibrated = true;
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
    start_time_calibrate_threshold = k_uptime_get();
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
        LOG_INF(" = %d mV", moisture_val_mv);
    }
}

void calibrate_soil_sensor()
{
    read_soil_moisture_mv();
    if (dry_plant_threshold == 0)
    {
        dry_plant_threshold = moisture_val_mv;
    }
    if (moisture_val_mv < lowest_mv)
    {
        lowest_mv = moisture_val_mv;
    }
    int64_t elapsed_ms = k_uptime_get() - start_time_calibrate_threshold;
    if (elapsed_ms >= 40000)
    {
        wet_plant_threshold = lowest_mv;
        LOG_INF(" wet threshold set: %d", wet_plant_threshold);
        soil_moisture_calibrated = true;
    }
}