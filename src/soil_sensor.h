#ifndef SOIL_SENSOR
#define SOIL_SENSOR

#include "sensor_config.h"

extern bool soil_moisture_calibrated;
extern int moisture_val_mv;
extern int smooth_soil_val;


extern enum SOIL_SENSOR_STATE CURRENT_SOIL_STATE;

void initialize_adc();
void read_soil_moisture_mv();
void calibrate_soil_sensor(CalibrationContext *ctx);
int mv_to_percentage(int value);
int smooth_data();
void read_smooth_soil();

#endif // SOIL_SENSOR