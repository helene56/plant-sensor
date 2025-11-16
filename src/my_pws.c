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
#include "sensor_config.h"
#include "logging_sensor.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(Lesson4_Exercise1);
// LOG_MODULE_DECLARE(Plant_sensor);

#define PUMP_ON_ARRAY_SIZE 5
// static bool notify_mysensor_enabled;
static bool notify_temperature_enabled;
static bool notify_calibration_enabled;

static uint32_t pumping_on_arr[PUMP_ON_ARRAY_SIZE];
static struct my_pws_cb pws_cb;
static uint32_t pws_data_logs[STORED_LOGS];

// TODO: i need to have a write handler for my timestamp value coming from the app on start up

static void mylbsbc_ccc_mysensor_cfg_changed(const struct bt_gatt_attr *attr,
											 uint16_t value)
{
	notify_temperature_enabled = (value == BT_GATT_CCC_NOTIFY); // and gives humidity..and also currently sending the soil moisture smooth data..
}

static void mylbsbc_ccc_calibration_cfg_changed(const struct bt_gatt_attr *attr,
											 uint16_t value)
{
	notify_calibration_enabled = (value == BT_GATT_CCC_NOTIFY);
}

// TODO: remove, but also update the attrs numbering otherwise will not work anymore
static ssize_t read_pump(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
						 uint16_t len, uint16_t offset)
{
	// get a pointer to pump_state which is passed in the BT_GATT_CHARACTERISTIC() and stored in attr->user_data
	const uint32_t *value = (const uint32_t *)attr->user_data;

	LOG_DBG("Attribute read, handle: %u, conn: %p", attr->handle, (void *)conn);

	if (pws_cb.pump_cb)
	{
		// Call the application callback function to update the get the current value of the pump
		const uint32_t *pump_values = pws_cb.pump_cb();
		// Copy values if needed:
		memcpy(pumping_on_arr, pump_values, sizeof(pumping_on_arr));
		return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(pumping_on_arr));
	}

	return 0;
}

static ssize_t read_log(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
						 uint16_t len, uint16_t offset)
{
	// get a pointer to pump_state which is passed in the BT_GATT_CHARACTERISTIC() and stored in attr->user_data
	const uint32_t *value = (const uint32_t *)attr->user_data;

	LOG_DBG("Attribute read, handle: %u, conn: %p", attr->handle, (void *)conn);

	if (pws_cb.update_logs_cb)
	{
		// Call the application callback function to update the get the current value of the pump
		const uint32_t *log_values = pws_cb.update_logs_cb();
		memcpy(pws_data_logs, log_values, sizeof(pws_data_logs));
		// TODO: set size of to only be the new data i want to send and the log_values should 
		// naturally be the start of the array where the new values are defined
		return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(pws_data_logs));
	}

	return 0;
}


static ssize_t write_command(struct bt_conn *conn,
							 const struct bt_gatt_attr *attr,
							 const void *buf,
							 uint16_t len, uint16_t offset, uint8_t flags)
{
	if (len != 1U)
	{
		LOG_DBG("Write command: Incorrect data length");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (offset != 0)
	{
		LOG_DBG("Write command: Incorrect data offset");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (pws_cb.sensor_command_cb)
	{
		// Read the received value
		uint8_t val = *((uint8_t *)buf);

		uint8_t on_off = (val >> 7) & 1; // on -> 1, off -> 0
		uint8_t id = val & 0x7F;		 // last 7 bits

		if (id < NUM_OF_CMDS && (on_off == 0x00 || on_off == 0x01))
		{
			// Call the application callback function to update the command state
			pws_cb.sensor_command_cb((bool)on_off, id);
		}
		else
		{
			LOG_DBG("Write command: Incorrect value");
			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}

		return len;
	}
	return 0;
}

static ssize_t write_timestamp(struct bt_conn *conn,
							 const struct bt_gatt_attr *attr,
							 const void *buf,
							 uint16_t len, uint16_t offset, uint8_t flags)
{
	// if (len != 1U)
	// {
	// 	LOG_DBG("Write command: Incorrect data length");
	// 	return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	// }

	if (offset != 0)
	{
		LOG_DBG("Write command: Incorrect data offset");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (pws_cb.init_time_stamp_cb)
	{
		// Read the received value
		// int64_t val = *((int64_t *)buf);
		int64_t val = (int64_t)sys_get_le64(buf);
		// update initial time stamp
		pws_cb.init_time_stamp_cb(val);

		return len;
	}

	return 0;
}

/* Plant Weather Service Declaration */
/* STEP 2 - Create and add the MY PWS service to the Bluetooth LE stack */
BT_GATT_SERVICE_DEFINE(my_pws_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_PWS),

					   /* STEP 4 - Create and add the Pump characteristic. */
					   BT_GATT_CHARACTERISTIC(BT_UUID_PWS_PUMP, BT_GATT_CHRC_READ,
											  BT_GATT_PERM_READ, read_pump, NULL, pumping_on_arr),
					   // temperature characteristic (also contains the humidity) TODO: consider renaming
					   BT_GATT_CHARACTERISTIC(BT_UUID_PWS_TEMPERATURE,
											  BT_GATT_CHRC_NOTIFY,
											  BT_GATT_PERM_NONE, NULL, NULL,
											  NULL),

					   BT_GATT_CCC(mylbsbc_ccc_mysensor_cfg_changed,
								   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

					   BT_GATT_CHARACTERISTIC(BT_UUID_PWS_SENSOR_COMMAND,
											  BT_GATT_CHRC_WRITE, BT_GATT_PERM_WRITE, NULL, write_command, NULL),
					   // notify
					   BT_GATT_CHARACTERISTIC(BT_UUID_PWS_CALIBRATION,
											  BT_GATT_CHRC_NOTIFY,
											  BT_GATT_PERM_NONE,
											  NULL, NULL, NULL),

					   BT_GATT_CCC(mylbsbc_ccc_calibration_cfg_changed,
								   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

						// 
						BT_GATT_CHARACTERISTIC(BT_UUID_PWS_LOG, BT_GATT_CHRC_READ,
											  BT_GATT_PERM_READ, read_log, NULL, pws_data_logs),
						// time stamp write handler
						BT_GATT_CHARACTERISTIC(BT_UUID_PWS_TIMESTAMP,
											  BT_GATT_CHRC_WRITE, BT_GATT_PERM_WRITE, NULL, write_timestamp, NULL)

);
/* A function to register application callbacks for the Pump and Temperature characteristics  */
int my_pws_init(struct my_pws_cb *callbacks)
{
	if (callbacks)
	{
		pws_cb.pump_cb = callbacks->pump_cb;
		pws_cb.sensor_command_cb = callbacks->sensor_command_cb;
		pws_cb.update_logs_cb = callbacks->update_logs_cb;
		pws_cb.init_time_stamp_cb = callbacks->init_time_stamp_cb;
	}

	return 0;
}

int my_pws_send_temperature_notify(uint16_t *sensor_value)
{
	if (!notify_temperature_enabled)
	{
		return -EACCES;
	}

	return bt_gatt_notify(NULL, &my_pws_svc.attrs[4],
						  sensor_value,
						  SENSOR_ARRAY_SIZE * sizeof(uint16_t));
}

int my_pws_send_calibration_notify(int8_t calib_value)
{
	if (!notify_calibration_enabled)
	{
		return -EACCES;
	}

	return bt_gatt_notify(NULL, &my_pws_svc.attrs[8],
						  &calib_value,
						  sizeof(calib_value));
}
