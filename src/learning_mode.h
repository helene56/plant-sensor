#ifndef LEARNING_MODE
#define LEARNING_MODE

#include <zephyr/kernel.h>

typedef struct
{
    int moisture_before; // before watering
    int moisture_after;  // after watering - measure 15 min. after
    uint32_t time_stamp;
    int hour_of_day; // maybe not needed?
    // int day_of_week; // maybe not needed?
    uint16_t temp;
} learning_data;

enum time_of_day
{
    MORNING,   // 06:00 - 12:00
    AFTERNOON, // 12:00 - 18:00
    EVENING,   // 18:00 - 00:00
    NIGHT,     // 00:00 - 06:00
    PERIOD_COUNTS
};

typedef struct
{
    int avg_drying_speed;
    enum time_of_day time_period;
    uint16_t avg_temp;

} learning_profile;

enum learning_phases
{
    LEARN,
    APPLY
};

typedef struct
{
    enum learning_phases phase;
    int log_period;
} learning_mode;

void phase_init();
void log_period_init(learning_mode current_mode);
uint32_t timestamp_ms_to_hour_of_day(uint32_t time_stamp);
int get_drying_speed(int moisture_previous, int moisture_current, int hours_passed);
learning_profile *get_learning_profile();
int predict_next_watering(int desired_moisture_per, int avg_drying_speed, int current_moisture);
void learning_time();
enum time_of_day get_time_of_day(int hour_of_day);

#endif /* LEARNING_MODE */