/*
 * ECU node firmware: periodic sensor read -> CAN tx, and CAN rx -> actuator logic.
 *
 * CAN protocol (standard 11-bit IDs):
 *   0x100 SENSOR_SPEED  - tx every 100ms, 2 bytes, little-endian counter (0..255)
 *   0x200 ACTUATOR_CMD  - rx, 1 byte, 0 = actuator off, nonzero = actuator on
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>

#define SENSOR_SPEED_ID 0x100
#define ACTUATOR_CMD_ID 0x200
#define SENSOR_PERIOD K_MSEC(100)

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

static void actuator_rx_callback(const struct device *dev, struct can_frame *frame,
				  void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	if (frame->dlc < 1) {
		return;
	}

	printk("actuator: %s\n", frame->data[0] != 0 ? "ON" : "OFF");
}

int main(void)
{
	const struct can_filter actuator_filter = {
		.id = ACTUATOR_CMD_ID,
		.mask = CAN_STD_ID_MASK,
	};
	uint16_t speed = 0;
	int err;

	if (!device_is_ready(can_dev)) {
		printk("CAN device not ready\n");
		return 0;
	}

	err = can_start(can_dev);
	if (err != 0) {
		printk("can_start failed: %d\n", err);
		return 0;
	}

	err = can_add_rx_filter(can_dev, actuator_rx_callback, NULL, &actuator_filter);
	if (err < 0) {
		printk("can_add_rx_filter failed: %d\n", err);
		return 0;
	}

	while (1) {
		struct can_frame frame = {
			.id = SENSOR_SPEED_ID,
			.dlc = 2,
		};

		sys_put_le16(speed, frame.data);

		err = can_send(can_dev, &frame, K_MSEC(100), NULL, NULL);
		if (err != 0) {
			printk("can_send failed: %d\n", err);
		}

		speed++;
		k_sleep(SENSOR_PERIOD);
	}

	return 0;
}
