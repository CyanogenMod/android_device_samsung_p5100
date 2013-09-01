/*
 * Copyright (C) 2013 Paul Kocialkowski
 *
 * Orientation calculation based on AK8975_FS:
 * Copyright (C) 2012 Asahi Kasei Microdevices Corporation, Japan
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <math.h>
#include <linux/ioctl.h>
#include <linux/input.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "orientationd.h"

#include <hardware/sensors.h>

#define ABS_CONTROL_REPORT	(ABS_THROTTLE)

#define FLAG_X		(1 << 0)
#define FLAG_Y		(1 << 1)
#define FLAG_Z		(1 << 2)
#define FLAG_ALL	(FLAG_X | FLAG_Y | FLAG_Z)

struct sensor_device {
	char *name;
	int handle;
	float (*get_data)(int value);
	int (*set_data)(float value);

	int fd;
};

struct sensor_data {
	vector v;
	int flags;
};

float rad2deg(float v)
{
	return (v * 180.0f / 3.1415926535f);
}

void orientation_calculate(vector *a, vector *m, vector *o)
{
	float azimuth, pitch, roll;
	float la, sinp, cosp, sinr, cosr, x, y;

	if (a == NULL || m == NULL || o == NULL)
		return;

	la = vector_length(a);
	pitch = asinf(-(a->y) / la);
	roll = asinf((a->x) / la);

	sinp = sinf(pitch);
	cosp = cosf(pitch);
	sinr = sinf(roll);
	cosr = cosf(roll);

	y = -(m->x) * cosr + m->z * sinr;
	x = m->x * sinp * sinr + m->y * cosp + m->z * sinp * cosr;
	azimuth = atan2f(y, x);

	o->x = rad2deg(azimuth);
	o->y = rad2deg(pitch);
	o->z = rad2deg(roll);

	if (o->x < 0)
		o->x += 360.0f;
}

float bma250_acceleration(int value)
{
	return (float) (value * GRAVITY_EARTH) / 256.0f;
}

float yas530c_magnetic(int value)
{
	return (float) value / 1000.0f;
}

int yas_orientation(float value)
{
	return (int) (value * 1000);
}

struct sensor_device bma250_device = {
	.name = "accelerometer",
	.handle = SENSOR_TYPE_ACCELEROMETER,
	.get_data = bma250_acceleration,
	.set_data = NULL,
	.fd = -1,
};

struct sensor_device yas530c_device = {
	.name = "geomagnetic",
	.handle = SENSOR_TYPE_MAGNETIC_FIELD,
	.get_data = yas530c_magnetic,
	.set_data = NULL,
	.fd = -1,
};

struct sensor_device yas_orientation_device = {
	.name = "orientation",
	.handle = SENSOR_TYPE_ORIENTATION,
	.get_data = NULL,
	.set_data = yas_orientation,
	.fd = -1,
};

struct sensor_device *sensor_devices[] = {
	&bma250_device,
	&yas530c_device,
	&yas_orientation_device,
};

int sensors_devices_count = sizeof(sensor_devices) / sizeof(struct sensor_device *);

int sensor_device_open(struct sensor_device *dev)
{
	int fd;

	if (dev == NULL || dev->name == NULL)
		return -EINVAL;

	printf("Opening %s\n", dev->name);

	fd = input_open(dev->name, dev->handle == SENSOR_TYPE_ORIENTATION ? 1 : 0);
	if (fd < 0)
		return -1;

	dev->fd = fd;

	return 0;
}

void sensor_device_close(struct sensor_device *dev)
{
	if (dev == NULL || dev->fd < 0)
		return;

	close(dev->fd);
	dev->fd = -1;
}

struct sensor_device *sensor_device_find_handle(int handle)
{
	int i;

	for (i=0 ; i < sensors_devices_count ; i++) {
		if (sensor_devices[i]->handle == handle)
			return sensor_devices[i];
	}

	return NULL;
}

struct sensor_device *sensor_device_find_fd(int fd)
{
	int i;

	for (i=0 ; i < sensors_devices_count ; i++) {
		if (sensor_devices[i]->fd == fd)
			return sensor_devices[i];
	}

	return NULL;
}

int sensor_device_get_data(struct sensor_device *dev, struct sensor_data *d,
	struct input_event *e)
{
	if (dev == NULL || d == NULL || e == NULL || dev->get_data == NULL)
		return -EINVAL;

	if (e->type == EV_ABS) {
		switch (e->code) {
			case ABS_X:
				d->v.x = dev->get_data(e->value);
				d->flags |= FLAG_X;
				return 0;
			case ABS_Y:
				d->v.y = dev->get_data(e->value);
				d->flags |= FLAG_Y;
				return 0;
			case ABS_Z:
				d->v.z = dev->get_data(e->value);
				d->flags |= FLAG_Z;
				return 0;
		}
	}

	return -1;
}

int sensor_device_set_data(struct sensor_device *dev, struct sensor_data *d)
{
	struct input_event event;

	if (dev == NULL || d == NULL || dev->set_data == NULL)
		return -EINVAL;

	event.type = EV_ABS;
	event.code = ABS_X;
	event.value = dev->set_data(d->v.x);
	gettimeofday(&event.time, NULL);
	write(dev->fd, &event, sizeof(event));

	event.type = EV_ABS;
	event.code = ABS_Y;
	event.value = dev->set_data(d->v.y);
	gettimeofday(&event.time, NULL);
	write(dev->fd, &event, sizeof(event));

	event.type = EV_ABS;
	event.code = ABS_Z;
	event.value = dev->set_data(d->v.z);
	gettimeofday(&event.time, NULL);
	write(dev->fd, &event, sizeof(event));

	event.type = EV_SYN;
	event.code = SYN_REPORT;
	event.value = 0;
	gettimeofday(&event.time, NULL);
	write(dev->fd, &event, sizeof(event));

	return 0;
}

int sensor_device_control(struct sensor_device *dev, struct input_event *e)
{
	int enabled;

	if (dev == NULL || e == NULL)
		return -EINVAL;

	if (e->type == EV_ABS && e->code == ABS_CONTROL_REPORT) {
		enabled = e->value & (1 << 16);
		if (enabled)
			return 1;
		else
			return 0;
	}

	return -1;
}

int main(int argc, char *argv[])
{
	struct input_event event;
	struct sensor_data a, m, o;

	struct sensor_device *dev;
	struct pollfd *poll_fds;

	int enabled, data;
	int index;

	int rc, c, i;

	memset(&a, 0, sizeof(a));
	memset(&m, 0, sizeof(m));
	memset(&o, 0, sizeof(o));

	poll_fds = (struct pollfd *) calloc(1, sizeof(struct pollfd) * sensors_devices_count);

	index = -1;
	c = 0;

	for (i=0 ; i < sensors_devices_count ; i++) {
		rc = sensor_device_open(sensor_devices[i]);
		if (rc < 0)
			continue;

		poll_fds[c].fd = sensor_devices[i]->fd;
		poll_fds[c].events = POLLIN;

		if (sensor_devices[i]->handle == SENSOR_TYPE_ORIENTATION && index < 0)
			index = c;

		c++;
	}

	if (c <= 0 || index <= 0)
		goto exit;

	printf("Starting main loop\n");

	enabled = 0;
	while (1) {
		data = 0;

		if (enabled)
			rc = poll(poll_fds, c, -1);
		else
			rc = poll(&poll_fds[index], 1, -1);

		if (rc < 0)
			goto exit;

		for (i=0 ; i < c ; i++) {
			if (poll_fds[i].revents & POLLIN) {
				dev = sensor_device_find_fd(poll_fds[i].fd);
				if (dev == NULL)
					continue;

				read(dev->fd, &event, sizeof(event));

				switch (dev->handle) {
					case SENSOR_TYPE_ACCELEROMETER:
						rc = sensor_device_get_data(dev, &a, &event);
						if (rc >= 0)
							data = 1;
						break;
					case SENSOR_TYPE_MAGNETIC_FIELD:
						rc = sensor_device_get_data(dev, &m, &event);
						if (rc >= 0)
							data = 1;
						break;
					case SENSOR_TYPE_ORIENTATION:
						rc = sensor_device_control(dev, &event);
						if (rc == 1)
							enabled = 1;
						else if (rc == 0)
							enabled = 0;
						break;
				}
			}

			if (data && a.flags & FLAG_ALL && m.flags & FLAG_ALL) {
				dev = sensor_device_find_handle(SENSOR_TYPE_ORIENTATION);
				if (dev == NULL)
					continue;

				orientation_calculate(&a.v, &m.v, &o.v);
				sensor_device_set_data(dev, &o);
			}
		}
	}


exit:
	for (i=0 ; i < sensors_devices_count ; i++)
		sensor_device_close(sensor_devices[i]);

	while (1) {
		sleep(3600);
	}

	return 0;
}
