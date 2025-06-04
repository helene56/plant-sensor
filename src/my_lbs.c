/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief LED Button Service (LBS) sample
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

static bool button_state;
static struct my_pws_cb pws_cb;


/* STEP 5 - Implement the read callback function of the Button characteristic */
static ssize_t read_temperature(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	// get a pointer to button_state which is passed in the BT_GATT_CHARACTERISTIC() and stored in attr->user_data
	const char *value = attr->user_data;

	LOG_DBG("Attribute read, handle: %u, conn: %p", attr->handle, (void *)conn);

	if (pws_cb.temperature_cb) {
		// Call the application callback function to update the get the current value of the button
		button_state = pws_cb.button_cb();
		return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(*value));
	}

	return 0;
}

/* LED Button Service Declaration */
/* STEP 2 - Create and add the MY PWS service to the Bluetooth LE stack */
BT_GATT_SERVICE_DEFINE(my_pws_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_PWS),
		       /* STEP 3 - Create and add the Temperature characteristic */
		       BT_GATT_CHARACTERISTIC(BT_UUID_PWS_TEMPERATURE, BT_GATT_CHRC_READ,
					      BT_GATT_PERM_READ, read_temperature, NULL, &temperature_state),
		       /* STEP 4 - Create and add the Pump characteristic. */
		       BT_GATT_CHARACTERISTIC(BT_UUID_PWS_PUMP, BT_GATT_CHRC_READ,
					      BT_GATT_PERM_READ, NULL, read_pump, &pump_state),

);
/* A function to register application callbacks for the LED and Button characteristics  */
int my_lbs_init(struct my_lbs_cb *callbacks)
{
	if (callbacks) {
		lbs_cb.led_cb = callbacks->led_cb;
		lbs_cb.button_cb = callbacks->button_cb;
	}

	return 0;
}
