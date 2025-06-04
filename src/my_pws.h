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
#define BT_UUID_PWS_TEMPERATURE_VAL                                                                     \
	BT_UUID_128_ENCODE(0x0f956142, 0x6b9c, 0x4a41, 0xa6df, 0x977ac4b99d78)

/** @brief Pump Characteristic UUID. */
#define BT_UUID_PWS_PUMP_VAL BT_UUID_128_ENCODE(0x0f956143, 0x6b9c, 0x4a41, 0xa6df, 0x977ac4b99d78)

#define BT_UUID_PWS BT_UUID_DECLARE_128(BT_UUID_PWS_VAL)
#define BT_UUID_PWS_TEMPERATURE BT_UUID_DECLARE_128(BT_UUID_PWS_TEMPERATURE_VAL)
#define BT_UUID_PWS_PUMP BT_UUID_DECLARE_128(BT_UUID_PWS_PUMP_VAL)

/** @brief Callback type for when an LED state change is received. */
typedef void (*led_cb_t)(const bool led_state);

/** @brief Callback type for when the button state is pulled. */
typedef bool (*button_cb_t)(void);

/** @brief Callback struct used by the LBS Service. */
struct my_lbs_cb {
	/** LED state change callback. */
	led_cb_t led_cb;
	/** Button read callback. */
	button_cb_t button_cb;
};

/** @brief Initialize the LBS Service.
 *
 * This function registers application callback functions with the My LBS
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
int my_lbs_init(struct my_lbs_cb *callbacks);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_PWS_H_ */
