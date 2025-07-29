#ifndef SOIL_SENSOR
#define SOIL_SENSOR

extern bool soil_moisture_calibrated;
extern int moisture_val_mv;

void initialize_adc();
void read_soil_moisture_mv();
void calibrate_soil_sensor();
int mv_to_percentage(int value);
int smooth_data();

#endif // SOIL_SENSOR