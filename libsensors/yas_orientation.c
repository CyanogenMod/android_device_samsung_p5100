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
#include <stddef.h>
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

struct yas_orientation_data {
	struct piranha_sensors_device *device;

	char path_enable[PATH_MAX];
	char path_delay[PATH_MAX];

	char acc_path_enable[PATH_MAX];
	char acc_path_delay[PATH_MAX];

	char mag_path_enable[PATH_MAX];
	char mag_path_delay[PATH_MAX];

	sensors_vec_t orientation;
};

int yas_orientation_init(struct piranha_sensors_handlers *handlers, struct piranha_sensors_device *device)
{
	struct yas_orientation_data *data = NULL;
	char path[PATH_MAX] = { 0 };
	int input_fd = -1;
	int rc;

	ALOGD("%s(%p, %p)", __func__, handlers, device);

	if (handlers == NULL || device == NULL)
		return -EINVAL;

	input_fd = input_open("orientation");
	if (input_fd < 0) {
		ALOGE("%s: Unable to open input", __func__);
		goto error;
	}

	rc = sysfs_path_prefix("orientation", (char *) &path);
	if (rc < 0 || path[0] == '\0') {
		ALOGE("%s: Unable to open sysfs", __func__);
		goto error;
	}

	data = (struct yas_orientation_data *) calloc(1, sizeof(struct yas_orientation_data));
	data->device = device;

	snprintf(data->path_enable, PATH_MAX, "%s/enable", path);
	snprintf(data->path_delay, PATH_MAX, "%s/delay", path);

	memset(&path, 0, sizeof(path));

	rc = sysfs_path_prefix("accelerometer", (char *) &path);
	if (rc < 0 || path[0] == '\0') {
		ALOGE("%s: Unable to open sysfs", __func__);
		goto error;
	}

	snprintf(data->acc_path_enable, PATH_MAX, "%s/enable", path);
	snprintf(data->acc_path_delay, PATH_MAX, "%s/delay", path);

	memset(&path, 0, sizeof(path));

	rc = sysfs_path_prefix("geomagnetic", (char *) &path);
	if (rc < 0 || path[0] == '\0') {
		ALOGE("%s: Unable to open sysfs", __func__);
		goto error;
	}

	snprintf(data->mag_path_enable, PATH_MAX, "%s/enable", path);
	snprintf(data->mag_path_delay, PATH_MAX, "%s/delay", path);

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

int yas_orientation_deinit(struct piranha_sensors_handlers *handlers)
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

int yas_orientation_activate(struct piranha_sensors_handlers *handlers)
{
	struct yas_orientation_data *data;
	char enable[] = "1\n";
	int fd;
	int rc;

	ALOGD("%s(%p)", __func__, handlers);

	if (handlers == NULL || handlers->data == NULL)
		return -EINVAL;

	data = (struct yas_orientation_data *) handlers->data;

	fd = open(data->acc_path_enable, O_WRONLY);
	if (fd < 0) {
		ALOGE("%s: Unable to open enable path", __func__);
		return -1;
	}

	write(fd, &enable, sizeof(enable));
	close(fd);

	fd = open(data->mag_path_enable, O_WRONLY);
	if (fd < 0) {
		ALOGE("%s: Unable to open enable path", __func__);
		return -1;
	}

	write(fd, &enable, sizeof(enable));
	close(fd);

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

int yas_orientation_deactivate(struct piranha_sensors_handlers *handlers)
{
	struct yas_orientation_data *data;
	char enable[] = "0\n";
	int fd;
	int i;

	ALOGD("%s(%p)", __func__, handlers);

	if (handlers == NULL || handlers->data == NULL)
		return -EINVAL;

	data = (struct yas_orientation_data *) handlers->data;

	fd = open(data->path_enable, O_WRONLY);
	if (fd < 0) {
		ALOGE("%s: Unable to open enable path", __func__);
		return -1;
	}

	write(fd, &enable, sizeof(enable));
	close(fd);

	for (i=0 ; i < data->device->handlers_count ; i++) {
		if (data->device->handlers[i] == NULL)
			continue;

		if (data->device->handlers[i]->handle == SENSOR_TYPE_ACCELEROMETER && !data->device->handlers[i]->activated) {
			fd = open(data->acc_path_enable, O_WRONLY);
			if (fd < 0) {
				ALOGE("%s: Unable to open enable path", __func__);
				continue;
			}

			write(fd, &enable, sizeof(enable));
			close(fd);
		} else if (data->device->handlers[i]->handle == SENSOR_TYPE_MAGNETIC_FIELD && !data->device->handlers[i]->activated) {
			fd = open(data->mag_path_enable, O_WRONLY);
			if (fd < 0) {
				ALOGE("%s: Unable to open enable path", __func__);
				continue;
			}

			write(fd, &enable, sizeof(enable));
			close(fd);
		}
	}

	handlers->activated = 0;

	return 0;
}

int yas_orientation_set_delay(struct piranha_sensors_handlers *handlers, int64_t delay)
{
	struct yas_orientation_data *data;
	char *value = NULL;
	int d;
	int c;
	int fd;
	int rc;

//	ALOGD("%s(%p, %ld)", __func__, handlers, (long int) delay);

	if (handlers == NULL || handlers->data == NULL)
		return -EINVAL;

	data = (struct yas_orientation_data *) handlers->data;

	if (delay < 1000000)
		d = 0;
	else
		d = (int) (delay / 1000000);

	c = asprintf(&value, "%d\n", d);

	fd = open(data->acc_path_delay, O_WRONLY);
	if (fd < 0) {
		ALOGE("%s: Unable to open delay path", __func__);
		return -1;
	}

	write(fd, value, c);
	close(fd);

	fd = open(data->mag_path_delay, O_WRONLY);
	if (fd < 0) {
		ALOGE("%s: Unable to open delay path", __func__);
		return -1;
	}

	write(fd, value, c);
	close(fd);

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

float yas_orientation_orientation(int value)
{
	return (float) value / 1000.f;
}

int yas_orientation_get_data(struct piranha_sensors_handlers *handlers,
	struct sensors_event_t *event)
{
	struct yas_orientation_data *data;
	struct input_event input_event;
	int input_fd;
	int flag;
	int rc;

	if (handlers == NULL || handlers->data == NULL || event == NULL)
		return -EINVAL;

	data = (struct yas_orientation_data *) handlers->data;

	input_fd = handlers->poll_fd;
	if (input_fd < 0)
		return -EINVAL;

	event->version = sizeof(struct sensors_event_t);
	event->sensor = handlers->handle;
	event->type = handlers->handle;

	event->orientation.x = data->orientation.x;
	event->orientation.y = data->orientation.y;
	event->orientation.z = data->orientation.z;
	event->orientation.status = SENSOR_STATUS_ACCURACY_MEDIUM;

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
				event->orientation.x = yas_orientation_orientation(input_event.value);
				break;
			case ABS_Y:
				flag |= FLAG_Y;
				event->orientation.y = yas_orientation_orientation(input_event.value);
				break;
			case ABS_Z:
				flag |= FLAG_Z;
				event->orientation.z = yas_orientation_orientation(input_event.value);
				break;
			default:
				continue;
		}
		event->timestamp = input_timestamp(&input_event);
	}

	if (data->orientation.x != event->orientation.x)
		data->orientation.x = event->orientation.x;
	if (data->orientation.y != event->orientation.y)
		data->orientation.y = event->orientation.y;
	if (data->orientation.z != event->orientation.z)
		data->orientation.z = event->orientation.z;

	return 0;
}

struct piranha_sensors_handlers yas_orientation = {
	.name = "YAS Orientation",
	.handle = SENSOR_TYPE_ORIENTATION,
	.init = yas_orientation_init,
	.deinit = yas_orientation_deinit,
	.activate = yas_orientation_activate,
	.deactivate = yas_orientation_deactivate,
	.set_delay = yas_orientation_set_delay,
	.get_data = yas_orientation_get_data,
	.activated = 0,
	.poll_fd = -1,
	.data = NULL,
};
