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
#include <math.h>
#include <linux/ioctl.h>
#include <linux/input.h>

#include <hardware/sensors.h>
#include <hardware/hardware.h>

#define LOG_TAG "piranha_sensors"
#include <utils/Log.h>

#include "piranha_sensors.h"

struct gp2a_proximity_data {
	char path_enable[PATH_MAX];
};

int gp2a_proximity_init(struct piranha_sensors_handlers *handlers, struct piranha_sensors_device *device)
{
	struct gp2a_proximity_data *data = NULL;
	char path[PATH_MAX] = { 0 };
	int input_fd = -1;
	int rc;

	ALOGD("%s(%p, %p)", __func__, handlers, device);

	if (handlers == NULL)
		return -EINVAL;

	input_fd = input_open("proximity_sensor");
	if (input_fd < 0) {
		ALOGE("%s: Unable to open input", __func__);
		goto error;
	}

	rc = sysfs_path_prefix("proximity_sensor", (char *) &path);
	if (rc < 0 || path[0] == '\0') {
		ALOGE("%s: Unable to open sysfs", __func__);
		goto error;
	}

	data = (struct gp2a_proximity_data *) calloc(1, sizeof(struct gp2a_proximity_data));

	snprintf(data->path_enable, PATH_MAX, "%s/enable", path);

	handlers->poll_fd = input_fd;
	handlers->data = (void *) data;

	return 0;

error:
	if (input_fd >= 0)
		close(input_fd);

	if (data != NULL)
		free(data);

	handlers->poll_fd = -1;
	handlers->data = NULL;

	return -1;
}

int gp2a_proximity_deinit(struct piranha_sensors_handlers *handlers)
{
	int input_fd;

	ALOGD("%s(%p)", __func__, handlers);

	if (handlers == NULL)
		return -EINVAL;

	input_fd = handlers->poll_fd;
	if (input_fd >= 0)
		close(input_fd);

	handlers->poll_fd = -1;

	if (handlers->data != NULL)
		free(handlers->data);

	handlers->data = NULL;

	return 0;
}

int gp2a_proximity_activate(struct piranha_sensors_handlers *handlers)
{
	struct gp2a_proximity_data *data;
	char enable[] = "1\n";
	int fd;

	ALOGD("%s(%p)", __func__, handlers);

	if (handlers == NULL || handlers->data == NULL)
		return -EINVAL;

	data = (struct gp2a_proximity_data *) handlers->data;

	fd = open(data->path_enable, O_WRONLY);
	if (fd < 0) {
		ALOGE("%s: Unable to open enable path", __func__);
		return -1;
	}

	write(fd, &enable, sizeof(enable));
	close(fd);

	handlers->activated = 1;

	return 0;
}

int gp2a_proximity_deactivate(struct piranha_sensors_handlers *handlers)
{
	struct gp2a_proximity_data *data;
	char enable[] = "0\n";
	int fd;

	ALOGD("%s(%p)", __func__, handlers);

	if (handlers == NULL || handlers->data == NULL)
		return -EINVAL;

	data = (struct gp2a_proximity_data *) handlers->data;

	fd = open(data->path_enable, O_WRONLY);
	if (fd < 0) {
		ALOGE("%s: Unable to open enable path", __func__);
		return -1;
	}

	write(fd, &enable, sizeof(enable));
	close(fd);

	handlers->activated = 0;

	return 0;
}

int gp2a_proximity_set_delay(struct piranha_sensors_handlers *handlers, int64_t delay)
{
	struct gp2a_proximity_data *data;
	char *value = NULL;
	int c;
	int fd;

//	ALOGD("%s(%p, %ld)", __func__, handlers, (long int) delay);

	if (handlers == NULL || handlers->data == NULL)
		return -EINVAL;

	return 0;
}

float gp2a_proximity_proximity(int value)
{
	return (float) value * 5.0f;
}

int gp2a_proximity_get_data(struct piranha_sensors_handlers *handlers,
	struct sensors_event_t *event)
{
	struct input_event input_event;
	int input_fd;
	int rc;

	if (handlers == NULL || event == NULL)
		return -EINVAL;

	input_fd = handlers->poll_fd;
	if (input_fd < 0)
		return -EINVAL;

	rc = read(input_fd, &input_event, sizeof(input_event));
	if (rc < (int) sizeof(input_event))
		return -EINVAL;

	if (input_event.type != EV_ABS || input_event.code != ABS_DISTANCE)
		return -1;

	event->version = sizeof(struct sensors_event_t);
	event->sensor = handlers->handle;
	event->type = handlers->handle;
	event->timestamp = input_timestamp(&input_event);
	event->distance = gp2a_proximity_proximity(input_event.value);

	return 0;
}

struct piranha_sensors_handlers gp2a_proximity = {
	.name = "GP2A Proximity",
	.handle = SENSOR_TYPE_PROXIMITY,
	.init = gp2a_proximity_init,
	.deinit = gp2a_proximity_deinit,
	.activate = gp2a_proximity_activate,
	.deactivate = gp2a_proximity_deactivate,
	.set_delay = gp2a_proximity_set_delay,
	.get_data = gp2a_proximity_get_data,
	.activated = 0,
	.poll_fd = -1,
	.data = NULL,
};
