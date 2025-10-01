#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

#include "logging_sensor.h"
#include "soil_sensor.h"

LOG_MODULE_DECLARE(Plant_sensor);

#define LOG_PERIOD_MIN 120 // log every 2 hours
#define TEST_LOG_PERIOD_MIN 1 // test logging every 1 min
// #define STORED_LOGS 62
int64_t init_time_stamp = 0;
int64_t init_uptime = 0;
bool pump_on = false;
bool test_pump = true;
uint32_t data_logs[STORED_LOGS] = {0};
uint32_t *ptr_data_logs = data_logs;
static uint32_t *ptr_end_data_logs = data_logs + STORED_LOGS;
int current_log_size = 0;
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
    if (moisture_level < 30 || test_pump) // if below 30%
    {
        LOG_INF("turn pump on");
        pump_on = true;
        water_used = 10; // 10 ml
        // TODO: might be interesting to send this info to the app so I know at what time it went below 30
    }
    // TODO: unix time is converted into seconds here. need to consider if it might be better to send as ms?
    return (struct plant_log_data){.time_stamp = (uint32_t) (get_unix_timestamp_ms() / 1000),
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

void my_work_handler(struct k_work *work)
{
    /* do the processing that needs to be done periodically */
    LOG_INF("Timer going off!");
    int64_t recieved_time = get_unix_timestamp_ms();
    LOG_INF("time stamp at timer = %lld", recieved_time);
    struct plant_log_data current_log = get_sensor_data();
    log_data(current_log);
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
    k_timer_start(&my_timer, K_MINUTES(0), K_MINUTES(TEST_LOG_PERIOD_MIN));
}
