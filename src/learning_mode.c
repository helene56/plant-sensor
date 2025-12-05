#include <zephyr/kernel.h>
#include "learning_mode.h"
#include "dht_sensor.h"
#include "soil_sensor.h"
// create a learning phase for each plant
// should eventually give individual watering recommendations and send suggestions to the app
// 1. collect data - moisture, timestamp, moisture before and after watering 
// 2. collect 7 days worth of data - learning profile
// 3. based on learning phase calculate drying speed how fast does moisture drop
// 4. adjust timer to check if plant should be watered based on predicted moisture
// 5. extra: maybe it should make a new learning provfile each month, so it can base new recommendations
//           even when seasons change
#define WAIT_TIME_MIN K_MINUTES(15)
#define LOG_PERIOD_MIN_DEFAULT 120
#define LOG_PERIOD_MIN_LEARN 60
#define DATA_COLLECTION_SIZE 24 * 7

int current_log_period;

// dynamic value, based on learnings..
int log_period_predict = 0;
// learning data collection
// save in nvs - dont want to start over if disconnected
learning_data weekly_data_collection[DATA_COLLECTION_SIZE] = {0};
// pointers for easy access to data
learning_data *weekly_ptr = weekly_data_collection;
learning_data *weekly_end_ptr = weekly_data_collection + DATA_COLLECTION_SIZE;

learning_mode mode;

void phase_init()
{
    mode.phase = LEARN;
    mode.log_period = LOG_PERIOD_MIN_LEARN;
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
            drying_speed = get_drying_speed(weekly_data_collection[i-1].moisture_before, 
            weekly_data_collection[i].moisture_before, current_log_period);
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

void learning_time()
{
    if (weekly_ptr < weekly_end_ptr)
    {
        weekly_ptr->time_stamp = (uint32_t)(get_unix_timestamp_ms() / 1000);
        weekly_ptr->hour_of_day = timestamp_ms_to_hour_of_day(weekly_ptr->time_stamp);
        struct air_metrics env_readings = read_temp_humidity();
        weekly_ptr->temp = env_readings.temp;
        int stable_reading;

        int i;
        for (i = 0; i < 100; i++)
        {
            stable_reading = read_smooth_soil();
            if (stable_reading)
            {
                stable_reading = 0;
                break;
            }
        }
        weekly_ptr->moisture_before = mv_to_percentage(smooth_soil_val);
        k_sleep(WAIT_TIME_MIN);
         for (i = 0; i < 100; i++)
        {
            stable_reading = read_smooth_soil();
            if (stable_reading)
            {
                break;
            }
        }
        weekly_ptr->moisture_after = mv_to_percentage(smooth_soil_val);
        weekly_ptr++;

    }
    else
    {
        // learning time is now over, time to apply learnings

    }
}