/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>

#include <zephyr/drivers/adc.h>

#include <dk_buttons_and_leds.h>
/* STEP 7 - Include the header file of MY LBS customer service */
#include "my_pws.h"
#include "dht_sensor.h"
#include "sensor_config.h"
#include "soil_sensor.h"

// static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));

static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
    (BT_LE_ADV_OPT_CONN |
     BT_LE_ADV_OPT_USE_IDENTITY), /* Connectable advertising and use identity address */
    800,                          /* Min Advertising Interval 500ms (800*0.625ms) */
    801,                          /* Max Advertising Interval 500.625ms (801*0.625ms) */
    NULL);                        /* Set to NULL for undirected advertising */

LOG_MODULE_REGISTER(Plant_sensor, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define CON_STATUS_LED DK_LED2

#define STACKSIZE 2048
#define PRIORITY 7
/* STEP 9.1 - Specify the button to monitor */
#define TEMPERATURE_BUTTON DK_BTN1_MSK // Should be replaced with a sensor, temp button
#define PUMP_BUTTON DK_BTN2_MSK        // Should be replaced with a pump, temp button

#define RUN_LED_BLINK_INTERVAL 1000
#define NOTIFY_INTERVAL 5 * 1000 // dont read too often, to avoid sensor from heating it self up
#define TURN_MOTOR_OFF_INTERVAL 500
#define PUMP_ON_ARRAY_SIZE 5

static uint64_t start_time; // Stores connection start timestamp
static bool app_pump_state;
static bool pumping_state;
static uint16_t sensor_value_holder[SENSOR_ARRAY_SIZE] = {0};
// static bool read_from_sensor = false;

static uint32_t pumping_on_arr[PUMP_ON_ARRAY_SIZE] = {0};
static uint32_t pumping_timestamp_arr[PUMP_ON_ARRAY_SIZE] = {0};
static uint32_t *pump_ptr = pumping_on_arr;
static uint32_t *pump_end = pumping_on_arr + PUMP_ON_ARRAY_SIZE;
static uint32_t *timestamp_ptr = pumping_timestamp_arr;
int smooth_soil_val = -1;
struct peripheral_cmd peripheral_cmds[NUM_OF_CMDS];

static struct k_work adv_work;
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),

};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_PWS_VAL),
};

static void adv_work_handler(struct k_work *work)
{
    int err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

    if (err)
    {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    printk("Advertising successfully started\n");
}
static void advertising_start(void)
{
    k_work_submit(&adv_work);
}
static void recycled_cb(void)
{
    printk("Connection object available from previous conn. Disconnect is complete!\n");
    advertising_start();
}

bool simulate_rain_drop_sensor()
{
    // should replace with real sensor at some point
    uint64_t elapsed_ms = k_uptime_get() - start_time;
    if (elapsed_ms >= 20000)
    {
        printk("raindrop detected\n");
        return true;
    }
    return false;
}

void init_peripheral_cmds()
{
    for (int i = 0; i < NUM_OF_CMDS; ++i)
    {
        peripheral_cmds[i].enabled = false;
        peripheral_cmds[i].id = (enum PERIPHERAL_CMD_IDS)i;
    }
}

static void simulate_output_water(void)
{
    while (1)
    {
        if (pumping_state)
        {
            // means the soil capacitor sensor said - soil is dry!
            // keep pump on until signal back from rain drop sensor at bottom of pot
            // add failsafe maybe after 1 min. it should turn off..
            int64_t elapsed_ms = k_uptime_get() - start_time;
            if (simulate_rain_drop_sensor() || elapsed_ms >= 60000)
            {
                start_time = k_uptime_get();
                pumping_state = !pumping_state;

                // set value in pumping array to the elapsed ms
                if (pump_ptr < pump_end)
                {
                    *pump_ptr = (uint32_t)elapsed_ms;
                    *timestamp_ptr = (uint32_t)k_uptime_get();

                    ++pump_ptr;
                    ++timestamp_ptr;
                }
                else
                {
                    // reinitialize array
                    memset(pumping_on_arr, 0, sizeof(pumping_on_arr));
                    memset(pumping_timestamp_arr, 0, sizeof(pumping_timestamp_arr));
                    // set pointers back
                    pump_ptr = pumping_on_arr;
                    timestamp_ptr = pumping_timestamp_arr;
                }

                // LOG_INF("Printing uptime:\n");
                // LOG_INF("milliseconds: %lld", k_uptime_get());
            }
        }
        k_sleep(K_MSEC(TURN_MOTOR_OFF_INTERVAL));
    }
}

static uint32_t *app_pump_cb(void)
{
    // TODO:
    // 1 make an array to store how long the pump stayed on, set max size of 5
    // 2 if no more space in array, reset (move pointer back and set all values to 0)
    // 3 update functions to send uint32_t values
    return pumping_on_arr;
}

static void app_sensor_command_cb(bool state, uint8_t id)
{

    peripheral_cmds[id].enabled = state;
}

/* STEP 10 - Declare a varaible app_callbacks of type my_pws_cb and initiate its members to the applications call back functions we developed in steps 8.2 and 9.2. */
static struct my_pws_cb app_callbacks = {
    .pump_cb = app_pump_cb,
    .sensor_command_cb = app_sensor_command_cb,
};

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
    if (has_changed & PUMP_BUTTON)
    {
        uint32_t user_button_state = button_state & PUMP_BUTTON;
        app_pump_state = user_button_state ? true : false;
        if (app_pump_state)
        {
            // if button clicked, turn pump on, and again turn off
            pumping_state = !pumping_state;
            if (pumping_state)
                start_time = k_uptime_get();
        }
    }
}

void send_data_thread(void)
{
    while (1)
    {
        if (peripheral_cmds[TEMPERATURE_HUMIDITY].enabled)
        {
            /* Send notification, the function sends notifications only if a client is subscribed */
            struct air_metrics env_readings = read_temp_humidity();
            // read voltage level of moisture in soil
            // if (soil_moisture_calibrated)
            // {
            //     smooth_soil_val = read_smooth_soil();
            // }

            // send notification for temp/humidity
            sensor_value_holder[0] = env_readings.temp;
            sensor_value_holder[1] = env_readings.humidity;
            // maybe add a range where the value doesnt change? the mv measured varies even if water level is not changing
            sensor_value_holder[2] = soil_moisture_calibrated ? smooth_soil_val : 0;
            if (smooth_soil_val != -1)
            {
                LOG_INF("percentage: %d\n", mv_to_percentage(smooth_soil_val));
                smooth_soil_val = -1;
            }

            my_pws_send_sensor_notify(sensor_value_holder);

            k_sleep(K_MSEC(NOTIFY_INTERVAL));
        }
        else
        {
            k_sleep(K_MSEC(100));
        }
    }
}

void calibration_soil(void)
{
    while (1)
    {

        if (!soil_moisture_calibrated)
        {
            calibrate_soil_sensor();
        }
        k_sleep(K_MSEC(100));
    }
}

void read_soil(void)
{
    while (1)
    {
        if (soil_moisture_calibrated)
        {
            read_smooth_soil();
        }
        if (smooth_soil_val != -1)
        {
            k_sleep(K_MSEC(1500));
        }
        k_sleep(K_MSEC(100));
    }
}

static void on_connected(struct bt_conn *conn, uint8_t err)
{
    if (err)
    {
        printk("Connection failed (err %u)\n", err);
        return;
    }

    printk("Connected\n");

    dk_set_led_on(CON_STATUS_LED);
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("Disconnected (reason %u)\n", reason);

    dk_set_led_off(CON_STATUS_LED);
}

struct bt_conn_cb connection_callbacks = {
    .connected = on_connected,
    .disconnected = on_disconnected,
    .recycled = recycled_cb,
};

static int init_button(void)
{
    int err;

    err = dk_buttons_init(button_changed);
    if (err)
    {
        printk("Cannot init buttons (err: %d)\n", err);
    }

    return err;
}

int main(void)
{
    int blink_status = 0;
    int err;

    LOG_INF("Starting Lesson 4 - Exercise 1 \n");
    init_peripheral_cmds();
    err = dk_leds_init();
    if (err)
    {
        LOG_ERR("LEDs init failed (err %d)\n", err);
        return -1;
    }

    err = init_button();
    if (err)
    {
        printk("Button init failed (err %d)\n", err);
        return -1;
    }

    err = bt_enable(NULL);
    if (err)
    {
        LOG_ERR("Bluetooth init failed (err %d)\n", err);
        return -1;
    }
    bt_conn_cb_register(&connection_callbacks);

    /* STEP 11 - Pass your application callback functions stored in app_callbacks to the MY PWS service */
    err = my_pws_init(&app_callbacks);
    if (err)
    {
        printk("Failed to init LBS (err:%d)\n", err);
        return -1;
    }
    LOG_INF("Bluetooth initialized\n");
    k_work_init(&adv_work, adv_work_handler);
    advertising_start();

    initialize_adc();

    for (;;)
    {

        dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
        k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
    }
}

K_THREAD_DEFINE(send_data_thread_id, STACKSIZE, send_data_thread, NULL, NULL,
                NULL, PRIORITY, 0, 0);

K_THREAD_DEFINE(send_data_thread_id1, STACKSIZE, calibration_soil, NULL, NULL,
                NULL, 8, 0, 0);

K_THREAD_DEFINE(send_data_thread_id2, STACKSIZE, simulate_output_water, NULL, NULL,
                NULL, 8, 0, 0);

K_THREAD_DEFINE(send_data_thread_id3, STACKSIZE, read_soil, NULL, NULL,
                NULL, 6, 0, 0);