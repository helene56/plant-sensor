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
    WAIT,
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
    bool soil_moisture_calibrated;
    bool pump_finished;
} CalibrationContext;

void init_peripheral_cmds();

#endif // SENSOR_CONFIG_H
