/** @file
 *  @brief Plant Weather Service (PWS)
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "my_pws.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(Lesson4_Exercise1);

static bool notify_mysensor_enabled;
// static int temperature_state;
static bool pump_state;
static struct my_pws_cb pws_cb;

static void mylbsbc_ccc_mysensor_cfg_changed(const struct bt_gatt_attr *attr,
 uint16_t value)
{
	notify_mysensor_enabled = (value == BT_GATT_CCC_NOTIFY);
}

/* STEP 5 - Implement the read callback function of the Temperature characteristic */
// static ssize_t read_temperature(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
// 			   uint16_t len, uint16_t offset)
// {
// 	// get a pointer to temperature_state which is passed in the BT_GATT_CHARACTERISTIC() and stored in attr->user_data
// 	const char *value = attr->user_data;

// 	LOG_DBG("Attribute read, handle: %u, conn: %p", attr->handle, (void *)conn);

// 	if (pws_cb.temperature_cb) {
// 		// Call the application callback function to update the get the current value of the temperature
// 		temperature_state = pws_cb.temperature_cb();
// 		return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(*value));
// 	}

// 	return 0;
// }

static ssize_t read_pump(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	// get a pointer to pump_state which is passed in the BT_GATT_CHARACTERISTIC() and stored in attr->user_data
	const char *value = attr->user_data;

	LOG_DBG("Attribute read, handle: %u, conn: %p", attr->handle, (void *)conn);

	if (pws_cb.pump_cb) {
		// Call the application callback function to update the get the current value of the pump
		pump_state = pws_cb.pump_cb();
		return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(*value));
	}

	return 0;
}

/* Plant Weather Service Declaration */
/* STEP 2 - Create and add the MY PWS service to the Bluetooth LE stack */
BT_GATT_SERVICE_DEFINE(my_pws_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_PWS),
		       /* STEP 3 - Create and add the Temperature characteristic */
		    //    BT_GATT_CHARACTERISTIC(BT_UUID_PWS_TEMPERATURE, BT_GATT_CHRC_READ,
			// 		      BT_GATT_PERM_READ, read_temperature, NULL, &temperature_state),
		       /* STEP 4 - Create and add the Pump characteristic. */
		       BT_GATT_CHARACTERISTIC(BT_UUID_PWS_PUMP, BT_GATT_CHRC_READ,
					      BT_GATT_PERM_READ, read_pump, NULL, &pump_state),

				BT_GATT_CHARACTERISTIC(BT_UUID_PWS_TEMPERATURE,
				BT_GATT_CHRC_NOTIFY,
				BT_GATT_PERM_NONE, NULL, NULL,
				NULL),
					
				BT_GATT_CCC(mylbsbc_ccc_mysensor_cfg_changed,
				BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

);
/* A function to register application callbacks for the Pump and Temperature characteristics  */
int my_pws_init(struct my_pws_cb *callbacks)
{
	if (callbacks) {
		pws_cb.pump_cb = callbacks->pump_cb;
		pws_cb.temperature_cb = callbacks->temperature_cb;
	}

	return 0;
}

int my_pws_send_sensor_notify(uint32_t sensor_value)
{
	if (!notify_mysensor_enabled) {
	return -EACCES;
	}  
	return bt_gatt_notify(NULL, &my_pws_svc.attrs[5], 
	&sensor_value,
	sizeof(sensor_value));
}
