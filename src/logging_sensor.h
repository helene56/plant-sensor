#ifndef LOGGING_SENSOR
#define LOGGING_SENSOR

extern int64_t init_time_stamp;

struct plant_log_data
{
    int64_t time_stamp;
    int water_used;
    int current_temp;
};

int64_t get_unix_timestamp_ms();
void init_timer();

#endif /* LOGGING_SENSOR */