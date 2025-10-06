#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

#define SENSOR_ARRAY_SIZE 3

enum PERIPHERAL_CMD_IDS
{
    TEMPERATURE_HUMIDITY,
    LIGHT,
    SOIL,
    PUMP,
    SOIL_CAL,
    CLEAR_LOG,
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
} CalibrationContext;

void init_peripheral_cmds();

enum TIME_STAMP_STATUS
{
    TIME_STAMP_NOT_RECIEVED,
    TIME_STAMP_RECIEVED
};

#endif // SENSOR_CONFIG_H
