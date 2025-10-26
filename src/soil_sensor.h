#ifndef SOIL_SENSOR
#define SOIL_SENSOR

#include "sensor_config.h"

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>

extern int moisture_val_mv;
extern int smooth_soil_val;




void initialize_adc();
void read_soil_moisture_mv();
void calibrate_soil_sensor(CalibrationContext *ctx, struct nvs_fs *fs);
int mv_to_percentage(int value);
int smooth_data();
int read_smooth_soil();
int check_stability(int current_val);

#endif // SOIL_SENSOR