#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#include "dht_sensor.h"


const struct device *dht = DEVICE_DT_GET(DT_NODELABEL(dht0));

struct air_metrics read_temp_humidity()
{
    struct sensor_value temp, humidity;
    sensor_sample_fetch(dht);
    sensor_channel_get(dht, SENSOR_CHAN_AMBIENT_TEMP, &temp);
    sensor_channel_get(dht, SENSOR_CHAN_HUMIDITY, &humidity);
    // combine int and decimal value
    uint16_t temp_encoded = (temp.val1 * 10) + (temp.val2 / 100000);
    uint16_t humidity_encoded = (humidity.val1 * 10) + (humidity.val2 / 100000);
    struct air_metrics ret = {.temp = temp_encoded, .humidity = humidity_encoded};

    return ret;
}