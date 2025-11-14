#ifndef LOGGING_SENSOR
#define LOGGING_SENSOR

#include "dht_sensor.h"

extern int64_t init_time_stamp;
extern int64_t init_uptime;
extern bool pump_on;
#define STORED_LOGS 672 // 12 logs per day * 4 * 7 = 1 month, now to hold date + (temp + water) = 2
// so 2 * 12 * 4 * 7
extern uint32_t *ptr_data_logs;
extern int current_log_size;

struct plant_log_data
{
    uint32_t time_stamp;
    int water_used;
    struct air_metrics env_readings;
    int soil_moisture_level;
};

extern uint32_t data_logs[STORED_LOGS];

int64_t get_unix_timestamp_ms();
void init_timer();
void handle_timer();
struct plant_log_data get_sensor_data();
void log_data(struct plant_log_data log);
#endif /* LOGGING_SENSOR */