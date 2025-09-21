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
int get_log_duration_min(int current_time_min, int log_time_min);
int get_current_min_s(int64_t time);
#endif /* LOGGING_SENSOR */