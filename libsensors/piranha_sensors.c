/*
 * Copyright (C) 2013 Paul Kocialkowski
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

#include <hardware/sensors.h>
#include <hardware/hardware.h>

#define LOG_TAG "piranha_sensors"
#include <utils/Log.h>

#include "piranha_sensors.h"

/*
 * Sensors list
 */

struct sensor_t piranha_sensors[] = {
	{ "BMA254 Acceleration Sensor", "Bosch", 1, SENSOR_TYPE_ACCELEROMETER,
		SENSOR_TYPE_ACCELEROMETER, 19.6f, 0.0383f, 0.13f, 10000, {}, },
	{ "YAS530 Magnetic Sensor", "Yamaha", 1, SENSOR_TYPE_MAGNETIC_FIELD,
		SENSOR_TYPE_MAGNETIC_FIELD, 800.0f, 0.3f, 4.0f, 10000, {}, },
	{ "YAS Orientation Sensor", "Yamaha", 1, SENSOR_TYPE_ORIENTATION,
		SENSOR_TYPE_ORIENTATION, 360.0f, 0.1f, 0.0f, 10000, {}, },
#ifdef TARGET_DEVICE_P5100
	{ "BH1721 Light Sensor", "ROHM", 1, SENSOR_TYPE_LIGHT,
		SENSOR_TYPE_LIGHT, 0.0f, 0.0f, 0.0f, 0, {}, },
#endif
#ifdef TARGET_DEVICE_P3100
	{ "GP2A Light Sensor", "SHARP", 1, SENSOR_TYPE_LIGHT,
		SENSOR_TYPE_LIGHT, 0.0f, 0.0f, 0.0f, 0, {}, },
	{ "GP2A Proximity Sensor", "SHARP", 1, SENSOR_TYPE_PROXIMITY,
		SENSOR_TYPE_PROXIMITY, 5.0f, 0.0f, 0.0f, 0, {}, },
#endif
};

int piranha_sensors_count = sizeof(piranha_sensors) / sizeof(struct sensor_t);

struct piranha_sensors_handlers *piranha_sensors_handlers[] = {
	&bma250,
	&yas530c,
	&yas_orientation,
#ifdef TARGET_DEVICE_P5100
	&bh1721,
#endif
#ifdef TARGET_DEVICE_P3100
	&gp2a_light,
	&gp2a_proximity,
#endif
};

int piranha_sensors_handlers_count = sizeof(piranha_sensors_handlers) /
	sizeof(struct piranha_sensors_handlers *);

/*
 * Piranha Sensors
 */

int piranha_sensors_activate(struct sensors_poll_device_t *dev, int handle, int enabled)
{
	struct piranha_sensors_device *device;
	int i;

	ALOGD("%s(%p, %d, %d)", __func__, dev, handle, enabled);

	if (dev == NULL)
		return -EINVAL;

	device = (struct piranha_sensors_device *) dev;

	if (device->handlers == NULL || device->handlers_count <= 0)
		return -EINVAL;

	for (i=0 ; i < device->handlers_count ; i++) {
		if (device->handlers[i] == NULL)
			continue;

		if (device->handlers[i]->handle == handle) {
			if (enabled && device->handlers[i]->activate != NULL)
				return device->handlers[i]->activate(device->handlers[i]);
			else if (!enabled && device->handlers[i]->deactivate != NULL)
				return device->handlers[i]->deactivate(device->handlers[i]);
		}
	}

	return -1;
}

int piranha_sensors_set_delay(struct sensors_poll_device_t *dev, int handle, int64_t ns)
{
	struct piranha_sensors_device *device;
	int i;

	ALOGD("%s(%p, %d, %ld)", __func__, dev, handle, (long int) ns);

	if (dev == NULL)
		return -EINVAL;

	device = (struct piranha_sensors_device *) dev;

	if (device->handlers == NULL || device->handlers_count <= 0)
		return -EINVAL;

	for (i=0 ; i < device->handlers_count ; i++) {
		if (device->handlers[i] == NULL)
			continue;

		if (device->handlers[i]->handle == handle && device->handlers[i]->set_delay != NULL)
			return device->handlers[i]->set_delay(device->handlers[i], ns);
	}

	return 0;
}

int piranha_sensors_poll(struct sensors_poll_device_t *dev,
	struct sensors_event_t* data, int count)
{
	struct piranha_sensors_device *device;
	int i, j;
	int c, n;
	int poll_rc, rc;

//	ALOGD("%s(%p, %p, %d)", __func__, dev, data, count);

	if (dev == NULL)
		return -EINVAL;

	device = (struct piranha_sensors_device *) dev;

	if (device->handlers == NULL || device->handlers_count <= 0 ||
		device->poll_fds == NULL || device->poll_fds_count <= 0)
		return -EINVAL;

	n = 0;

	do {
		poll_rc = poll(device->poll_fds, device->poll_fds_count, PIRANHA_POLL_DELAY);
		if (poll_rc < 0)
			return -1;

		for (i=0 ; i < device->poll_fds_count ; i++) {
			if (!(device->poll_fds[i].revents & POLLIN))
				continue;

			for (j=0 ; j < device->handlers_count ; j++) {
				if (device->handlers[j] == NULL || device->handlers[j]->poll_fd != device->poll_fds[i].fd || device->handlers[j]->get_data == NULL)
					continue;

				rc = device->handlers[j]->get_data(device->handlers[j], &data[n]);
				if (rc < 0) {
					device->poll_fds[i].revents = 0;
					poll_rc = -1;
				} else {
					n++;
					count--;
				}
			}

			if (count <= 0)
				break;
		}
	} while ((poll_rc > 0 || n < 1) && count > 0);

	return n;
}

/*
 * Interface
 */

int piranha_sensors_close(hw_device_t *device)
{
	struct piranha_sensors_device *piranha_sensors_device;
	int i;

	ALOGD("%s(%p)", __func__, device);

	if (device == NULL)
		return -EINVAL;

	piranha_sensors_device = (struct piranha_sensors_device *) device;

	if (piranha_sensors_device->poll_fds != NULL)
		free(piranha_sensors_device->poll_fds);

	for (i=0 ; i < piranha_sensors_device->handlers_count ; i++) {
		if (piranha_sensors_device->handlers[i] == NULL || piranha_sensors_device->handlers[i]->deinit == NULL)
			continue;

		piranha_sensors_device->handlers[i]->deinit(piranha_sensors_device->handlers[i]);
	}

	free(device);

	return 0;
}

int piranha_sensors_open(const struct hw_module_t* module, const char *id,
	struct hw_device_t** device)
{
	struct piranha_sensors_device *piranha_sensors_device;
	int p, i;

	ALOGD("%s(%p, %s, %p)", __func__, module, id, device);

	if (module == NULL || device == NULL)
		return -EINVAL;

	piranha_sensors_device = (struct piranha_sensors_device *)
		calloc(1, sizeof(struct piranha_sensors_device));
	piranha_sensors_device->device.common.tag = HARDWARE_DEVICE_TAG;
	piranha_sensors_device->device.common.version = 0;
	piranha_sensors_device->device.common.module = (struct hw_module_t *) module;
	piranha_sensors_device->device.common.close = piranha_sensors_close;
	piranha_sensors_device->device.activate = piranha_sensors_activate;
	piranha_sensors_device->device.setDelay = piranha_sensors_set_delay;
	piranha_sensors_device->device.poll = piranha_sensors_poll;
	piranha_sensors_device->handlers = piranha_sensors_handlers;
	piranha_sensors_device->handlers_count = piranha_sensors_handlers_count;
	piranha_sensors_device->poll_fds = (struct pollfd *)
		calloc(1, piranha_sensors_handlers_count * sizeof(struct pollfd));

	p = 0;
	for (i=0 ; i < piranha_sensors_handlers_count ; i++) {
		if (piranha_sensors_handlers[i] == NULL || piranha_sensors_handlers[i]->init == NULL)
			continue;

		piranha_sensors_handlers[i]->init(piranha_sensors_handlers[i], piranha_sensors_device);
		if (piranha_sensors_handlers[i]->poll_fd >= 0) {
			piranha_sensors_device->poll_fds[p].fd = piranha_sensors_handlers[i]->poll_fd;
			piranha_sensors_device->poll_fds[p].events = POLLIN;
			p++;
		}
	}

	piranha_sensors_device->poll_fds_count = p;

	*device = &(piranha_sensors_device->device.common);

	return 0;
}

int piranha_sensors_get_sensors_list(struct sensors_module_t* module,
	const struct sensor_t **sensors_p)
{
	ALOGD("%s(%p, %p)", __func__, module, sensors_p);

	if (sensors_p == NULL)
		return -EINVAL;

	*sensors_p = piranha_sensors;
	return piranha_sensors_count;
}

struct hw_module_methods_t piranha_sensors_module_methods = {
	.open = piranha_sensors_open,
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
	.common = {
		.tag = HARDWARE_MODULE_TAG,
		.version_major = 1,
		.version_minor = 0,
		.id = SENSORS_HARDWARE_MODULE_ID,
		.name = "Piranha Sensors",
		.author = "Paul Kocialkowski",
		.methods = &piranha_sensors_module_methods,
	},
	.get_sensors_list = piranha_sensors_get_sensors_list,
};
