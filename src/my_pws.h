#ifndef BT_PWS_H_
#define BT_PWS_H_

/**@file
 * @defgroup bt_pws Plant Weather Service Service API
 * @{
 * @brief API for the Plant Weather Service (PWS).
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>

/* STEP 1 - Define the 128 bit UUIDs for the GATT service and its characteristics in */
/** @brief PWS Service UUID. */
#define BT_UUID_PWS_VAL BT_UUID_128_ENCODE(0x0f956141, 0x6b9c, 0x4a41, 0xa6df, 0x977ac4b99d78)

/** @brief Temperature Characteristic UUID. */
// TODO: should probably rename this uuid, as it sends temp and humidity, probably also a good idea to send other infos..
// rename to environment readings?
#define BT_UUID_PWS_TEMPERATURE_VAL                                                                     \
	BT_UUID_128_ENCODE(0x0f956142, 0x6b9c, 0x4a41, 0xa6df, 0x977ac4b99d78)

/** @brief Pump Characteristic UUID. */
#define BT_UUID_PWS_PUMP_VAL BT_UUID_128_ENCODE(0x0f956143, 0x6b9c, 0x4a41, 0xa6df, 0x977ac4b99d78)

#define BT_UUID_PWS_SENSOR_COMMAND_VAL                                                                     \
	BT_UUID_128_ENCODE(0x0f956144, 0x6b9c, 0x4a41, 0xa6df, 0x977ac4b99d78)

#define BT_UUID_PWS_CALIBRATION_VAL                                                                     \
	BT_UUID_128_ENCODE(0x0f956145, 0x6b9c, 0x4a41, 0xa6df, 0x977ac4b99d78)

#define BT_UUID_PWS_LOG_VAL                                                                     \
	BT_UUID_128_ENCODE(0x0f956146, 0x6b9c, 0x4a41, 0xa6df, 0x977ac4b99d78)

#define BT_UUID_PWS_TIMESTAMP_VAL                                                                     \
	BT_UUID_128_ENCODE(0x0f956147, 0x6b9c, 0x4a41, 0xa6df, 0x977ac4b99d78)

#define BT_UUID_PWS BT_UUID_DECLARE_128(BT_UUID_PWS_VAL)
#define BT_UUID_PWS_TEMPERATURE BT_UUID_DECLARE_128(BT_UUID_PWS_TEMPERATURE_VAL)
#define BT_UUID_PWS_PUMP BT_UUID_DECLARE_128(BT_UUID_PWS_PUMP_VAL)
#define BT_UUID_PWS_SENSOR_COMMAND BT_UUID_DECLARE_128(BT_UUID_PWS_SENSOR_COMMAND_VAL)
#define BT_UUID_PWS_CALIBRATION BT_UUID_DECLARE_128(BT_UUID_PWS_CALIBRATION_VAL)
#define BT_UUID_PWS_LOG BT_UUID_DECLARE_128(BT_UUID_PWS_LOG_VAL)
#define BT_UUID_PWS_TIMESTAMP BT_UUID_DECLARE_128(BT_UUID_PWS_TIMESTAMP_VAL)

/** @brief Callback type for when an pump state change is received. */
typedef uint32_t* (*pump_cb_t)(void);

/** @brief Callback type for when the sensor_command state is pulled. */
typedef void (*sensor_command_cb_t)(bool, uint8_t);

typedef uint32_t* (*update_logs_cb_t)(void);

typedef void (*init_time_stamp_cb_t)(int64_t);

/** @brief Callback struct used by the PWS Service. */
struct my_pws_cb {
	/** pump state change callback. */
	pump_cb_t pump_cb;
	/** sensor_command read callback. */
	sensor_command_cb_t sensor_command_cb;
	// update logging - TODO: temporary function to send out a log
	update_logs_cb_t update_logs_cb;
	// write timestamp
	init_time_stamp_cb_t init_time_stamp_cb;
};

/** @brief Initialize the PWS Service.
 *
 * This function registers application callback functions with the My PWS
 * Service
 *
 * @param[in] callbacks Struct containing pointers to callback functions
 *			used by the service. This pointer can be NULL
 *			if no callback functions are defined.
 *
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int my_pws_init(struct my_pws_cb *callbacks);

/** @brief Send the sensor value as notification.
 *
 * This function sends an uint16_t  value, typically the value
 * of a simulated sensor to all connected peers.
 *
 * @param[in] sensor_value The value of the simulated sensor.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int my_pws_send_temperature_notify(uint16_t *sensor_value);
int my_pws_send_calibration_notify(int8_t calib_value);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_PWS_H_ */
