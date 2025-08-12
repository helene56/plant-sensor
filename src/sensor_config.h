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

void init_peripheral_cmds();

#endif // SENSOR_CONFIG_H
