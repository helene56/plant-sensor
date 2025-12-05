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
#define DATA_COLLECTION_SIZE 24 * 7

int current_log_period;


typedef struct
{
    int moisture_before; // before watering
    int moisture_after; // after watering - measure 15 min. after
    uint32_t time_stamp;
    int hour_of_day; // maybe not needed?
    int day_of_week; // maybe not needed?
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

enum time_of_day get_time_of_day(int hour_of_day)
{
    return ((hour_of_day - 6 + 24) % 24) / 6;
}

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


// dynamic value, based on learnings..
int log_period_predict = 0;
// learning data collection
learning_data weekly_data_collection[DATA_COLLECTION_SIZE] = {0};

void phase_init()
{
    learning_mode mode =
    {
        .phase = LEARN,
        .log_period = LOG_PERIOD_MIN_LEARN
    };
}

void log_period_init(learning_mode current_mode)
{
    current_log_period = current_mode.log_period;
}

uint32_t timestamp_ms_to_hour_of_day (uint32_t time_stamp)
{
    time_stamp = time_stamp / 1000;
    uint32_t seconds_in_day = time_stamp % 86400;
    return seconds_in_day / 3600;
}

int get_drying_speed(int moisture_previous, int moisture_current, int hours_passed)
{
    return (moisture_previous - moisture_current) / hours_passed;
}

learning_profile *get_learning_profile()
{
    learning_profile monthly_profile[4] = {0};
    int count[PERIOD_COUNTS] = {0};

    for (int i = 0; i<DATA_COLLECTION_SIZE; ++i)
    {
        enum time_of_day period = get_time_of_day(weekly_data_collection[i].hour_of_day);
        int drying_speed;
        if (i == 0)
        {
            drying_speed = 0;
        }
        else
        {
            drying_speed = get_drying_speed(weekly_data_collection[i-1].moisture_after, 
            weekly_data_collection[i].moisture_after, current_log_period);
        }
        
        monthly_profile[period].avg_drying_speed += drying_speed;
        monthly_profile[period].avg_temp += weekly_data_collection[i].temp;
        count[period] += 1;

    }

    for (int p = 0; p < PERIOD_COUNTS; ++p)
    {
        if (count[p] > 0)
        {
            monthly_profile[p].avg_drying_speed /= count[p];
            monthly_profile[p].avg_temp /= count[p];
        }
        
    }

    return monthly_profile;
}

int predict_next_watering(int desired_moisture_per, int avg_drying_speed, int current_moisture)
{
    int moisture_drop = current_moisture - desired_moisture_per;
    if (moisture_drop <= 0)
    {
        return 0;
    }
    return moisture_drop / avg_drying_speed;
}