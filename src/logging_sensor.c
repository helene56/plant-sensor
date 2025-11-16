#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

#include "sensor_config.h"
#include "logging_sensor.h"
#include "soil_sensor.h"

LOG_MODULE_DECLARE(Plant_sensor);

#define LOG_PERIOD_MIN 120    // log every 2 hours
#define TEST_LOG_PERIOD_MIN 1 // test logging every 1 min

int64_t init_time_stamp = 0;
int64_t init_uptime = 0;
bool pump_on = false;
bool test_pump = true;
uint32_t data_logs[STORED_LOGS] = {0};
uint32_t *ptr_data_logs = data_logs;
static uint32_t *ptr_end_data_logs = data_logs + STORED_LOGS;
int current_log_size = 0;
bool START_LOGGING = false;
// timer
bool START_TIMER = false;
// send 1. date, 2. water_used, 3. temperature, 4. soil moisture (own level system?)
// evaluate if pump should turn on based on this info.

// for now water_used will have to be estimated, as i dont have a flow sensor yet.
// also dont have a sensor to sense water at bottom of pot yet, so will only turn pump on for a set amount of time.

struct plant_log_data get_sensor_data(CalibrationContext *ctx)
{

    // get temp and humidity
    struct air_metrics env_readings = read_temp_humidity();
    // get up to date soil val
    int stable_reading;

    int i;
    for (i = 0; i < 100; i++)
    {
        stable_reading = read_smooth_soil();
        if (stable_reading)
        {
            break;
        }
    }
    LOG_INF("Stable reading achieved at loop %d", i);

    int moisture_level = mv_to_percentage(smooth_soil_val);
    int water_used = 0;
    // TODO: it takes a while for the sensor to register the moisture
    // but this should not be a problem with a logging every 2 hours..
    // but the data readings might not have been stabilized yet.. smooth_soil_val is just the latest reading.
    if (moisture_level < 30) // if below 30%
    {
        LOG_INF("turn pump on");
        peripheral_cmds[PUMP].enabled = true;
        water_used = 10; // 10 ml
        // TODO: might be interesting to send this info to the app so I know at what time it went below 30
    }
    // TODO: unix time is converted into seconds here. need to consider if it might be better to send as ms?
    return (struct plant_log_data){.time_stamp = (uint32_t)(get_unix_timestamp_ms() / 1000),
                                   .env_readings = env_readings,
                                   .soil_moisture_level = moisture_level,
                                   .water_used = water_used};
}

void log_data(struct plant_log_data log)
{
    // log data
    if (ptr_data_logs < ptr_end_data_logs)
    {

        *ptr_data_logs = log.time_stamp;
        ptr_data_logs++;
        *ptr_data_logs = (log.env_readings.temp << 16) | log.water_used;
        ptr_data_logs++;
    }
}

int64_t get_unix_timestamp_ms()
{

    int64_t current_uptime = k_uptime_get() - init_uptime;
    return init_time_stamp + current_uptime;
}

void my_timer_handler(struct k_timer *dummy)
{
    // stop the timer thread
    START_TIMER = false;
    // start the logging thread
    START_LOGGING = true;
}

K_TIMER_DEFINE(my_timer, my_timer_handler, NULL);
void init_timer()
{
    LOG_INF("Starting first log");
    k_timer_start(&my_timer, K_MINUTES(TEST_LOG_PERIOD_MIN), K_MINUTES(0));
    START_TIMER = false;
}

void handle_timer()
{
    LOG_INF("Restarting timer.");
    k_timer_start(&my_timer, K_MINUTES(TEST_LOG_PERIOD_MIN), K_MINUTES(0));
}
