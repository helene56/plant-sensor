#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

#include "logging_sensor.h"
#include "soil_sensor.h"

LOG_MODULE_DECLARE(Plant_sensor);

#define LOG_PERIOD_MIN 120 // log every 2 hours
#define STORED_LOGS 62
int64_t init_time_stamp = 0;
int64_t init_uptime = 0;
static struct plant_log_data data_logs[STORED_LOGS] = {0};
static struct plant_log_data *ptr_data_logs = data_logs;
// send 1. date, 2. water_used, 3. temperature, 4. soil moisture (own level system?)
// evaluate if pump should turn on based on this info.

// for now water_used will have to be estimated, as i dont have a flow sensor yet.
// also dont have a sensor to sense water at bottom of pot yet, so will only turn pump on for a set amount of time.

struct plant_log_data get_sensor_data()
{

    // get temp and humidity
    struct air_metrics env_readings = read_temp_humidity();
    // get up to date soil val
    read_smooth_soil();
    int moisture_level = mv_to_percentage(smooth_soil_val);
    int water_used = 0;
    if (moisture_level < 30) // if below 30%
    {
        LOG_INF("turn pump on");
        water_used = 10; // 10 ml
    }
    return (struct plant_log_data){.time_stamp = get_unix_timestamp_ms(),
                                   .env_readings = env_readings,
                                   .soil_moisture_level = moisture_level,
                                   .water_used = water_used};
}

void log_data(struct plant_log_data log)
{
    // log data
    *ptr_data_logs = log;
}

// temp function till i can get a rtc module
// on start up i need to request the current unix date
int64_t get_unix_timestamp_ms()
{

    int64_t current_uptime = k_uptime_get() - init_uptime;
    return init_time_stamp + current_uptime;
}

void my_work_handler(struct k_work *work)
{
    /* do the processing that needs to be done periodically */
    LOG_INF("Timer going off!");
}

K_WORK_DEFINE(my_work, my_work_handler);

void my_timer_handler(struct k_timer *dummy)
{
    k_work_submit(&my_work);
}

K_TIMER_DEFINE(my_timer, my_timer_handler, NULL);
void init_timer()
{
    LOG_INF("Starting first log");
    k_timer_start(&my_timer, K_MINUTES(0), K_MINUTES(LOG_PERIOD_MIN));
}
