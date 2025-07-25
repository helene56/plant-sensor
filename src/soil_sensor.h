#ifndef SOIL_SENSOR
#define SOIL_SENSOR

extern bool soil_moisture_calibrated;
extern int moisture_val_mv;

void initialize_adc();
void read_soil_moisture_mv();
void calibrate_soil_sensor();

#endif // SOIL_SENSOR