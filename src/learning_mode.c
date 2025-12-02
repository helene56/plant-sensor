#include <zephyr/kernel.h>
// create a learning phase for each plant
// should eventually give individual watering recommendations and send suggestions to the app
// 1. collect data - moisture, timestamp, moisture before and after watering 
// 2. collect 7 days worth of data - learning profile
// 3. based on learning phase calculate drying speed how fast does moisture drop
// 4. adjust timer to check if plant should be watered based on predicted moisture
// 5. extra: maybe it should make a new learning provfile each month, so it can base new recommendations
//           even when seasons change
#define LOG_PERIOD_MIN_DEFAULT 120
#define LOG_PERIOD_MIN_LEARN 60

// dynamic value, based on learnings..
int log_period_predict = 0;

typedef struct
{
    int moisture_before;
    int moisture_after;
    uint32_t time_stamp;
    int hour_of_day; // maybe not needed?
    int day_of_week;
} learning_data;

enum time_of_day
{
    MORNING,
    AFTERNOON,
    EVENING,
    NIGHT
};

typedef struct
{
    int avg_drying_speed;
    enum time_of_day time_period;
    uint16_t temp;

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


void phase_init()
{
    learning_mode mode =
    {
        .phase = LEARN,
        .log_period = LOG_PERIOD_MIN_LEARN
    };
}