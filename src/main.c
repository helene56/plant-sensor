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
#include <zephyr/drivers/gpio.h>

// write to flash
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>


#include "my_pws.h"
#include "dht_sensor.h"
#include "sensor_config.h"
#include "soil_sensor.h"
#include "logging_sensor.h"

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

#define RUN_LED_BLINK_INTERVAL 1000
#define NOTIFY_INTERVAL 5 * 1000 // dont read too often, to avoid sensor from heating it self up
#define TURN_MOTOR_OFF_INTERVAL 500


// write to flash
static struct nvs_fs fs;

#define NVS_PARTITION		storage_partition
#define NVS_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(NVS_PARTITION)

#define RBT_CNT_ID 0
uint32_t reboot_counter = 0U;
bool soil_moisture_calibrate_status = false;
int dry_plant_threshold = 0;
int wet_plant_threshold = 0;
int ideal_plant_threshold = 0;


void init_nvs_counter(struct nvs_fs *fs, uint16_t id, int *data, size_t len, const char *data_name)
{
    int rc = 0;
    rc = nvs_read(fs, id, data, len);
    if (rc > 0) { /* item was found, show it */
        printk("Id: %d, %s: %d\n",
            id, data_name, *data);
    } else   {/* item was not found, add it */
        printk("No %s found, adding it at id %d\n",
               data_name, id);
        (void)nvs_write(fs, id, data,
              len);
    }

}

void init_nvs(CalibrationContext *ctx)
{
    int rc = 0;
    struct flash_pages_info info;

    /* define the nvs file system by settings with:
     *	sector_size equal to the pagesize,
     *	3 sectors
     *	starting at NVS_PARTITION_OFFSET
     */
    fs.flash_device = NVS_PARTITION_DEVICE;
    if (!device_is_ready(fs.flash_device)) {
        printk("Flash device %s is not ready\n", fs.flash_device->name);
        return;
    }
    fs.offset = NVS_PARTITION_OFFSET;
    rc = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);
    if (rc) {
        printk("Unable to get page info\n");
        return;
    }
    fs.sector_size = info.size;
    fs.sector_count = 2U;

    rc = nvs_mount(&fs);
    if (rc) {
        printk("Flash Init failed\n");
        return;
    }

    /* RBT_CNT_ID is used to store the reboot counter, lets see
     * if we can read it from flash
     */
    rc = nvs_read(&fs, RBT_CNT_ID, &reboot_counter, sizeof(reboot_counter));
    if (rc > 0) { /* item was found, show it */
        printk("Id: %d, Reboot_counter: %d\n",
            RBT_CNT_ID, reboot_counter);
    } else   {/* item was not found, add it */
        printk("No Reboot counter found, adding it at id %d\n",
               RBT_CNT_ID);
        (void)nvs_write(&fs, RBT_CNT_ID, &reboot_counter,
              sizeof(reboot_counter));
    }

    // initialize uninitialized values here..
    // soil_moisture_calibrate_status
    
    rc = nvs_read(&fs, SOIL_MOI_CAL_ID, &soil_moisture_calibrate_status, sizeof(soil_moisture_calibrate_status));
    if (rc > 0) { /* item was found, show it */
        printk("Id: %d, soil cal status: %d\n",
            SOIL_MOI_CAL_ID, soil_moisture_calibrate_status);
    } else   {/* item was not found, add it */
        printk("No soil cal found, adding it at id %d\n",
               SOIL_MOI_CAL_ID);
        (void)nvs_write(&fs, SOIL_MOI_CAL_ID, &soil_moisture_calibrate_status,
              sizeof(soil_moisture_calibrate_status));
    }

    // thresholds
    init_nvs_counter(&fs, DRY_PLANT_ID, &dry_plant_threshold, 
        sizeof(dry_plant_threshold), "dry plant threshold");
    init_nvs_counter(&fs, WET_PLANT_ID, &wet_plant_threshold, 
        sizeof(wet_plant_threshold), "wet plant threshold");
    init_nvs_counter(&fs, IDEAL_PLANT_ID, &ideal_plant_threshold, 
        sizeof(ideal_plant_threshold), "ideal plant threshold");

    // update calibrationcontext
    if (soil_moisture_calibrate_status)
    {
        // is already calibrated so this can be set to true immediatly
        ctx->soil_moisture_sensor_enabled = true;
    }

}


static uint64_t start_time; // Stores connection start timestamp
static uint16_t plant_env_readings[SENSOR_ARRAY_SIZE] = {0};
static bool print_once = true;
static CalibrationContext ctx = {
    .pump_finished = false,
    .pump_state = PUMP_OFF,
    .soil_moisture_ready_to_calibrate = false,
    .soil_moisture_sensor_enabled = false};

static const struct gpio_dt_spec pump = GPIO_DT_SPEC_GET(DT_ALIAS(pump0), gpios);

int smooth_soil_val = 0;
struct peripheral_cmd peripheral_cmds[NUM_OF_CMDS];

bool start_soil_sensor = false;

static uint32_t app_data_logs[62];

enum TIME_STAMP_STATUS current_time_stamp = TIME_STAMP_NOT_RECIEVED;

enum CALIBRATION_STATUSES
{
    DRY_FINISH = 1,
    IDEAL_FINISH,
    WAIT_TIME
};

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

void gpio_pump_init(void)
{
    // static const struct gpio_dt_spec pump = GPIO_DT_SPEC_GET(DT_ALIAS(pump0), gpios);

    if (!gpio_is_ready_dt(&pump))
    {
        return;
    }

    gpio_pin_configure_dt(&pump, GPIO_OUTPUT);
    gpio_pin_set_dt(&pump, 0);
}

void init_peripheral_cmds()
{
    for (int i = 0; i < NUM_OF_CMDS; ++i)
    {
        peripheral_cmds[i].enabled = false;
        peripheral_cmds[i].id = (enum PERIPHERAL_CMD_IDS)i;
    }
}

static void app_sensor_command_cb(bool state, uint8_t id)
{
    peripheral_cmds[id].enabled = state;
}

static uint32_t *app_update_logs()
{
    // // unix format -> 2025/08/26 at 12:36:00
    // app_data_logs[0] = 1756211760;
    // // two random values packed in order of LSB
    // app_data_logs[1] = (55 << 16) | 25;
    // TODO: only send new data
    // also send the size of the new data
    return data_logs;
}

static void app_init_time_stamp(int64_t time_stamp)
{
    // set time from recieved time from app
    // should only do so on start up once
    if (current_time_stamp == TIME_STAMP_NOT_RECIEVED)
    {
        init_time_stamp = time_stamp;
        init_uptime = k_uptime_get();
        current_time_stamp = TIME_STAMP_RECIEVED;
        LOG_INF("recieved timestamp");
        int64_t recieved_time = get_unix_timestamp_ms();
        LOG_INF("time stamp = %lld", recieved_time);
        // init_timer();
    }
    else
    {
        LOG_INF("timestamp already recieved");
    }
}

static void app_erase_logs()
{
    while (1)
    {
        // TODO: move a pointer to the next aviailbe space instead
        // keep the logic of clearing the logs to 0 for debugging purposes?
        if (peripheral_cmds[CLEAR_LOG].enabled)
        {
            for (int i = 0; i < STORED_LOGS; ++i)
            {
                app_data_logs[i] = 0;
            }
            peripheral_cmds[CLEAR_LOG].enabled = false;
            // reset array to start
            // set new
            ptr_data_logs = data_logs;
        }
        k_sleep(K_MSEC(100));
    }
}

/* STEP 10 - Declare a varaible app_callbacks of type my_pws_cb and initiate its members to the applications call back functions we developed in steps 8.2 and 9.2. */
static struct my_pws_cb app_callbacks = {
    .sensor_command_cb = app_sensor_command_cb,
    .update_logs_cb = app_update_logs,
    .init_time_stamp_cb = app_init_time_stamp};

void send_data_thread(void *p1)
{
    CalibrationContext *ctx = (CalibrationContext *)p1;
    while (1)
    {
        if (peripheral_cmds[TEMPERATURE_HUMIDITY].enabled)
        {
            /* Send notification, the function sends notifications only if a client is subscribed */
            struct air_metrics env_readings = read_temp_humidity();
            // send notification for temp/humidity
            plant_env_readings[0] = env_readings.temp;
            plant_env_readings[1] = env_readings.humidity;
          
            if (ctx->soil_moisture_sensor_enabled)
            {
                int stable_soil_reading = read_smooth_soil();
                if (stable_soil_reading)
                {
                    plant_env_readings[2] = mv_to_percentage(smooth_soil_val);
                }
            }
            else
            {
                plant_env_readings[2] = -1;
            }

            // maybe add a range where the value doesnt change? the mv measured varies even if water level is not changing
            // plant_env_readings[2] = ctx->soil_moisture_sensor_enabled ? mv_to_percentage(smooth_soil_val) : 0;
            if (ctx->soil_moisture_sensor_enabled)
            {
                LOG_INF("percentage: %d of soil moisture", mv_to_percentage(smooth_soil_val));
            }

            my_pws_send_temperature_notify(plant_env_readings);

            k_sleep(K_MSEC(NOTIFY_INTERVAL));
        }
        else
        {
            k_sleep(K_MSEC(100));
        }
    }
}

void start_pump()
{
    static bool set_start_time = false;
    static int64_t start_time_pump;
    while (1)
    {
        if (pump_on)
        {
            if (!set_start_time)
            {
                gpio_pin_set_dt(&pump, 1);
                start_time_pump = k_uptime_get();
                set_start_time = true;
            }
            else
            {
                int64_t elapsed_ms = k_uptime_get() - start_time_pump;
                // should only be on for 20 sec.
                if (elapsed_ms >= 20000)
                {
                    gpio_pin_set_dt(&pump, 0);
                    LOG_INF("stop watering..");
                    pump_on = false;
                    set_start_time = false;
                }
            }
        }
        k_sleep(K_MSEC(100));
    }
}

void manage_pump(CalibrationContext *ctx)
{
    if (peripheral_cmds[PUMP].enabled)
    {
        LOG_INF("watering..");
        gpio_pin_set_dt(&pump, 1);
        start_time = k_uptime_get();
        ctx->pump_state = PUMP_ON;
        peripheral_cmds[PUMP].enabled = false;
    }
    else if (ctx->pump_state == PUMP_ON)
    {

        int64_t elapsed_ms = k_uptime_get() - start_time;
        // should only be on for 5 sec.
        if (elapsed_ms >= 500)
        {
            gpio_pin_set_dt(&pump, 0);
            LOG_INF("stop watering..");
            ctx->pump_state = PUMP_OFF;
            ctx->pump_finished = true;
        }
    }
}

void do_calibration_step(CalibrationContext *ctx, struct nvs_fs *fs)
{
    switch (ctx->current_soil_state)
    {
    case DRY:
        calibrate_soil_sensor(ctx, fs);
        break;

    case START_PUMP:
        manage_pump(ctx);
        break;

    case WET:
        calibrate_soil_sensor(ctx, fs);
        break;

    case IDEAL:
        calibrate_soil_sensor(ctx, fs);
        break;
    }
}

void update_state(CalibrationContext *ctx)
{
    switch (ctx->current_soil_state)
    {
    case DRY:
        if (peripheral_cmds[SOIL_CAL].enabled && ctx->soil_moisture_ready_to_calibrate)
        {
            ctx->current_soil_state = START_PUMP;
            LOG_INF("moisture sensor calibrated in dry soil\n");
            my_pws_send_calibration_notify((int8_t)DRY_FINISH);
            LOG_INF("waiting for user to start pump..");
        }
        break;
    case START_PUMP:
        if (ctx->pump_finished)
        {
            ctx->current_soil_state = WET;
            ctx->soil_moisture_ready_to_calibrate = false;
            LOG_INF("pump finished. starting wet calibration.");
        }
        break;
    case WET:
        if (ctx->pump_finished && ctx->soil_moisture_ready_to_calibrate)
        {
            ctx->current_soil_state = IDEAL;
            LOG_INF("moisture sensor calibrated in wet soil\n");
            // moisture sensor should be calibrated
            ctx->pump_finished = false;
            // reset to make it possible to redo calibration
            ctx->soil_moisture_ready_to_calibrate = false;
        }
        break;
    case IDEAL:
        if (ctx->soil_moisture_ready_to_calibrate)
        {
            printk("ideal soil finish\n");
            // resetting
            ctx->current_soil_state = DRY;
            // TODO: set to true to enable reading from sensor at an interval?
            ctx->soil_moisture_ready_to_calibrate = false;
            peripheral_cmds[SOIL_CAL].enabled = false;
            printk("Calibration complete!\n");
            my_pws_send_calibration_notify((int8_t)IDEAL_FINISH);
            // ready to start the timer which logs
            init_timer();
            print_once = true;
            ctx->soil_moisture_sensor_enabled = true;
            // set to true and save in nvs
            soil_moisture_calibrate_status = true;
            (void)nvs_write(
                &fs, SOIL_MOI_CAL_ID, &soil_moisture_calibrate_status,
                sizeof(soil_moisture_calibrate_status));
        }
        break;
    }
}

// this should be the thread running the main calibration
void main_calibrate_thread(void *p1, struct nvs_fs *fs)
{
    CalibrationContext *ctx = (CalibrationContext *)p1;

    while (1)
    {
        if (peripheral_cmds[SOIL_CAL].enabled)
        {
            if (print_once)
            {
                LOG_INF("starting calibration");
                print_once = false;
                // reset to false, to avoid measuring of soil moisture while
                // calibration is ongoing
                ctx->soil_moisture_sensor_enabled = false;
            }
            do_calibration_step(ctx, fs);
            update_state(ctx);
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
    gpio_pump_init();
    // counter write to flash
    init_nvs(&ctx);
    reboot_counter++;
    (void)nvs_write(
        &fs, RBT_CNT_ID, &reboot_counter,
        sizeof(reboot_counter));


    for (;;)
    {

        dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
        k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
    }
}

K_THREAD_DEFINE(send_data_thread_id, STACKSIZE, send_data_thread, &ctx, &fs,
                NULL, 8, 0, 0);

K_THREAD_DEFINE(send_data_thread_id1, STACKSIZE, main_calibrate_thread, &ctx, &fs,
                NULL, PRIORITY, 0, 0);

K_THREAD_DEFINE(send_data_thread_id2, STACKSIZE, start_pump, NULL, NULL,
                NULL, 8, 0, 0);

// K_THREAD_DEFINE(send_data_thread_id3, STACKSIZE, read_soil, &ctx, NULL,
//                 NULL, 6, 0, 0);

K_THREAD_DEFINE(send_data_thread_id4, STACKSIZE, app_erase_logs, NULL, NULL,
                NULL, PRIORITY, 0, 0);
