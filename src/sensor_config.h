#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

#define SENSOR_ARRAY_SIZE 3

// nvs
#define SOIL_MOI_CAL_ID 1
extern bool soil_moisture_calibrate_status;
// // thresholds
#define DRY_PLANT_ID 2
#define WET_PLANT_ID 3
#define IDEAL_PLANT_ID 4
extern int dry_plant_threshold;
extern int wet_plant_threshold;
extern int ideal_plant_threshold;

enum PERIPHERAL_CMD_IDS
{
    TEMPERATURE_HUMIDITY,
    LIGHT,
    SOIL,
    PUMP,
    SOIL_CAL,
    CLEAR_LOG,
    TEST_PUMP, 
    NUM_OF_CMDS // used to define total amount of commands
};

struct peripheral_cmd
{
    bool enabled;
    enum PERIPHERAL_CMD_IDS id;
};

extern struct peripheral_cmd peripheral_cmds[NUM_OF_CMDS];

enum SOIL_SENSOR_STATE
{
    DRY,
    START_PUMP,
    WET,
    IDEAL,
};

enum PUMP_STATE
{
    PUMP_OFF,
    PUMP_ON,
};

typedef struct
{
    enum PUMP_STATE pump_state;
    enum SOIL_SENSOR_STATE current_soil_state;
    bool soil_moisture_ready_to_calibrate; // to tell whenever each soil_sensor_state has been completed.
    bool soil_moisture_sensor_enabled; // once soil moisture has been calibrated, it is now ready to submit readings.
    bool pump_finished;
    bool soil_moisture_calibrated_once; // first time calibration done, add here??
} CalibrationContext;

void init_peripheral_cmds();

enum TIME_STAMP_STATUS
{
    TIME_STAMP_NOT_RECIEVED,
    TIME_STAMP_RECIEVED
};

#endif // SENSOR_CONFIG_H
