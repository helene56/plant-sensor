#ifndef SOIL_SENSOR
#define SOIL_SENSOR

#include "sensor_config.h"

extern int moisture_val_mv;
extern int smooth_soil_val;




void initialize_adc();
void read_soil_moisture_mv();
void calibrate_soil_sensor(CalibrationContext *ctx);
int mv_to_percentage(int value);
int smooth_data();
int read_smooth_soil();
int check_stability(int current_val);

#endif // SOIL_SENSOR