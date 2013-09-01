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

#include <stdint.h>
#include <poll.h>

#include <linux/input.h>

#include <hardware/sensors.h>
#include <hardware/hardware.h>

#ifndef _SENSORS_H_
#define _SENSORS_H_

#define PIRANHA_POLL_DELAY	100

struct piranha_sensors_device;

struct piranha_sensors_handlers {
	char *name;
	int handle;

	int (*init)(struct piranha_sensors_handlers *handlers, struct piranha_sensors_device *device);
	int (*deinit)(struct piranha_sensors_handlers *handlers);
	int (*activate)(struct piranha_sensors_handlers *handlers);
	int (*deactivate)(struct piranha_sensors_handlers *handlers);
	int (*set_delay)(struct piranha_sensors_handlers *handlers, int64_t delay);
	int (*get_data)(struct piranha_sensors_handlers *handlers, struct sensors_event_t *event);

	int activated;
	int poll_fd;

	void *data;
};

struct piranha_sensors_device {
	struct sensors_poll_device_t device;

	struct piranha_sensors_handlers **handlers;
	int handlers_count;

	struct pollfd *poll_fds;
	int poll_fds_count;
};

extern struct piranha_sensors_handlers *piranha_sensors_handlers[];
extern int piranha_sensors_handlers_count;

int piranha_sensors_activate(struct sensors_poll_device_t *dev, int handle, int enabled);
int piranha_sensors_set_delay(struct sensors_poll_device_t *dev, int handle, int64_t ns);
int piranha_sensors_poll(struct sensors_poll_device_t *dev,
	struct sensors_event_t* data, int count);

/*
 * Input
 */

int64_t input_timestamp(struct input_event *event);
int input_open(char *name);
int sysfs_path_prefix(char *name, char *path_prefix);

/*
 * Sensors
 */

extern struct piranha_sensors_handlers bma250;
extern struct piranha_sensors_handlers yas530c;
extern struct piranha_sensors_handlers yas_orientation;
extern struct piranha_sensors_handlers bh1721;
extern struct piranha_sensors_handlers gp2a_light;
extern struct piranha_sensors_handlers gp2a_proximity;

#endif
