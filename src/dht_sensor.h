#ifndef DHT_SENSOR
#define DHT_SENSOR

#include <stdio.h>

struct air_metrics
{
    uint16_t temp;
    uint16_t humidity;
};

struct air_metrics read_temp_humidity();


#endif /* DHT_SENSOR */