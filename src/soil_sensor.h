#ifndef SOIL_SENSOR
#define SOIL_SENSOR

extern bool soil_moisture_calibrated;
extern int moisture_val_mv;
extern int smooth_soil_val;

enum SOIL_SENSOR_STATE
{
    DRY,
    WET,
    IDEAL,
};

extern enum SOIL_SENSOR_STATE CURRENT_SOIL_STATE;


void initialize_adc();
void read_soil_moisture_mv();
void calibrate_soil_sensor();
int mv_to_percentage(int value);
int smooth_data();
void read_smooth_soil();

#endif // SOIL_SENSOR