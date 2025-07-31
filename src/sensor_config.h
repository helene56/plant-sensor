#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

#define SENSOR_ARRAY_SIZE  3

struct peripheral_command_ids {
    uint8_t temp_id;
    uint8_t humidity_id;
    uint8_t light_id;
    uint8_t soil_id;
    uint8_t pump_id;
};

extern const struct peripheral_command_ids ble_commands;

struct status_command {
    uint8_t on_off : 1; // on -> 1, off -> 0
    uint8_t id : 7; // 7 bits left for id
};

#endif // SENSOR_CONFIG_H
