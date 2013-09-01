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
#include <linux/ioctl.h>
#include <linux/input.h>

#include <hardware/sensors.h>
#include <hardware/hardware.h>

#define LOG_TAG "piranha_sensors"
#include <utils/Log.h>

#include "piranha_sensors.h"

#define FLAG_X		(1 << 0)
#define FLAG_Y		(1 << 1)
#define FLAG_Z		(1 << 2)
#define FLAG_ALL	(FLAG_X | FLAG_Y | FLAG_Z)

struct yas530c_data {
	char path_enable[PATH_MAX];
	char path_delay[PATH_MAX];

	sensors_vec_t magnetic;
};

int yas530c_init(struct piranha_sensors_handlers *handlers, struct piranha_sensors_device *device)
{
	struct yas530c_data *data = NULL;
	char path[PATH_MAX] = { 0 };
	int input_fd = -1;
	int rc;

	ALOGD("%s(%p, %p)", __func__, handlers, device);

	if (handlers == NULL)
		return -EINVAL;

	input_fd = input_open("geomagnetic");
	if (input_fd < 0) {
		ALOGE("%s: Unable to open input", __func__);
		goto error;
	}

	rc = sysfs_path_prefix("geomagnetic", (char *) &path);
	if (rc < 0 || path[0] == '\0') {
		ALOGE("%s: Unable to open sysfs", __func__);
		goto error;
	}

	data = (struct yas530c_data *) calloc(1, sizeof(struct yas530c_data));

	snprintf(data->path_enable, PATH_MAX, "%s/enable", path);
	snprintf(data->path_delay, PATH_MAX, "%s/delay", path);

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

int yas530c_deinit(struct piranha_sensors_handlers *handlers)
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

int yas530c_activate(struct piranha_sensors_handlers *handlers)
{
	struct yas530c_data *data;
	char enable[] = "1\n";
	int fd;

	ALOGD("%s(%p)", __func__, handlers);

	if (handlers == NULL || handlers->data == NULL)
		return -EINVAL;

	data = (struct yas530c_data *) handlers->data;

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

int yas530c_deactivate(struct piranha_sensors_handlers *handlers)
{
	struct yas530c_data *data;
	char enable[] = "0\n";
	int fd;

	ALOGD("%s(%p)", __func__, handlers);

	if (handlers == NULL || handlers->data == NULL)
		return -EINVAL;

	data = (struct yas530c_data *) handlers->data;

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

int yas530c_set_delay(struct piranha_sensors_handlers *handlers, int64_t delay)
{
	struct yas530c_data *data;
	char *value = NULL;
	int d;
	int c;
	int fd;

//	ALOGD("%s(%p, %ld)", __func__, handlers, (long int) delay);

	if (handlers == NULL || handlers->data == NULL)
		return -EINVAL;

	data = (struct yas530c_data *) handlers->data;

	if (delay < 1000000)
		d = 0;
	else
		d = (int) (delay / 1000000);

	c = asprintf(&value, "%d\n", d);

	fd = open(data->path_delay, O_WRONLY);
	if (fd < 0) {
		ALOGE("%s: Unable to open delay path", __func__);
		return -1;
	}

	write(fd, value, c);
	close(fd);

	if (value != NULL)
		free(value);

	return 0;
}

float yas530c_magnetic(int value)
{
	return (float) value / 1000.0f;
}

int yas530c_get_data(struct piranha_sensors_handlers *handlers,
	struct sensors_event_t *event)
{
	struct yas530c_data *data;
	struct input_event input_event;
	int input_fd;
	int flag;
	int rc;

	if (handlers == NULL || handlers->data == NULL || event == NULL)
		return -EINVAL;

	data = (struct yas530c_data *) handlers->data;

	input_fd = handlers->poll_fd;
	if (input_fd < 0)
		return -EINVAL;

	event->version = sizeof(struct sensors_event_t);
	event->sensor = handlers->handle;
	event->type = handlers->handle;

	event->magnetic.x = data->magnetic.x;
	event->magnetic.y = data->magnetic.y;
	event->magnetic.z = data->magnetic.z;
	event->magnetic.status = SENSOR_STATUS_ACCURACY_MEDIUM;

	flag = 0;
	while ((flag & FLAG_ALL) != FLAG_ALL) {
		rc = read(input_fd, &input_event, sizeof(input_event));
		if (rc < (int) sizeof(input_event)) {
			if (flag & FLAG_ALL)
				break;
			else
				return -EINVAL;
		}

		if (input_event.type != EV_ABS)
			continue;

		switch (input_event.code) {
			case ABS_X:
				flag |= FLAG_X;
				event->magnetic.x = yas530c_magnetic(input_event.value);
				break;
			case ABS_Y:
				flag |= FLAG_Y;
				event->magnetic.y = yas530c_magnetic(input_event.value);
				break;
			case ABS_Z:
				flag |= FLAG_Z;
				event->magnetic.z = yas530c_magnetic(input_event.value);
				break;
			default:
				continue;
		}
		event->timestamp = input_timestamp(&input_event);
	}

	if (data->magnetic.x != event->magnetic.x)
		data->magnetic.x = event->magnetic.x;
	if (data->magnetic.y != event->magnetic.y)
		data->magnetic.y = event->magnetic.y;
	if (data->magnetic.z != event->magnetic.z)
		data->magnetic.z = event->magnetic.z;

	return 0;
}

struct piranha_sensors_handlers yas530c = {
	.name = "YAS530C",
	.handle = SENSOR_TYPE_MAGNETIC_FIELD,
	.init = yas530c_init,
	.deinit = yas530c_deinit,
	.activate = yas530c_activate,
	.deactivate = yas530c_deactivate,
	.set_delay = yas530c_set_delay,
	.get_data = yas530c_get_data,
	.activated = 0,
	.poll_fd = -1,
	.data = NULL,
};
