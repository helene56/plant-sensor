#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

#include "logging_sensor.h"

LOG_MODULE_DECLARE(Plant_sensor);

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

