#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

#include "logging_sensor.h"

LOG_MODULE_DECLARE(Plant_sensor);

// 1757743200

int64_t init_time_stamp = 0;
// send 1. date, 2. water_used, 3. temperature
// for now water_used will have to be estimated, as i dont have a flow sensor yet.
// also dont have a sensor to sense water at bottom of pot yet, so will only turn pump on for a set amount of time.

// void send_log_data(struct plant_log_data log)
// {

// }

// temp function till i can get a rtc module
// on start up i need to request the current unix date
int64_t get_unix_timestamp_ms()
{
    // k_ticks_to_cyc_ceil64(k_uptime_get());
    int64_t current_uptime = k_uptime_get();
    return init_time_stamp - current_uptime;
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
    k_timer_start(&my_timer, K_SECONDS(120), K_SECONDS(120));
}

