#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

#include "logging_sensor.h"
#include "soil_sensor.h"

LOG_MODULE_DECLARE(Plant_sensor);

#define LOG_PERIOD 24 // log every 24 hour at x time
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
    // need to set timer off at 8, 10, 12, 14, 16, 18
    // should have a timer for each?
    /* start a periodic timer that expires once every second */
    // timer for 8:00 (ca.)
    // 1. get the current time; example currently 12:32
    // 2. calculate the next time it is 8:00, duration
    // 3. set period to the next time it is 8:00, so again in 24 hours

    // test with minutes
    int current_min = get_current_min_s(get_unix_timestamp_ms());
    int log_min = 10; // at 40 min
    // how long till current_min is at log_min?
    int log_duration = get_log_duration_min(current_min, log_min);
    int log_period = 2; // next log should happen 2 min from 40 min
    LOG_INF("Current minutes is: %d", current_min);
    LOG_INF("Time till first timer setoff: %d", log_duration);
    LOG_INF("Time for going of next time: %d", log_period);
    k_timer_start(&my_timer, K_MINUTES(log_duration), K_MINUTES(log_period));
}

// temp function, should return the hour
int get_current_min_s(int64_t time_ms)
{
    // convert time_ms to s
    int64_t time_s = time_ms /= 1000;
    return (int)((time_s % 3600) / 60);
}
// temp, should be for the hour instead
int get_log_duration_min(int current_time_min, int log_time_min)
{
    if (current_time_min <= log_time_min)
    {
        return log_time_min - current_time_min;
    }
    else
    {
        return 60 - current_time_min + log_time_min;
    }
}