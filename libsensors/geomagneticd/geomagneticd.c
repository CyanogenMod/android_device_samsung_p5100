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
#include <dirent.h>
#include <sys/stat.h>
#include <linux/ioctl.h>
#include <linux/input.h>

#define LOG_TAG "geomagneticd"
#include <utils/Log.h>

/*
 * This is a very intuitive implementation of what's going on with p5100/p3100
 * geomagneticd daemon. It seemed that geomagneticd sets an offset so that
 * the biggest value (after setting the offset) is 45µT or negative -45µT.
 * On the X axis, it happens more often to find the max around 40µT/-40µT.
 * The reference offsets I used were: 5005 420432 1153869, and we're getting
 * pretty close to this with that implementation.
 *
 */

/*
 * Input
 */

int input_open(char *name)
{
	DIR *d;
	struct dirent *di;

	char input_name[80] = { 0 };
	char path[PATH_MAX];
	char *c;
	int fd;
	int rc;

	if (name == NULL)
		return -EINVAL;

	d = opendir("/dev/input");
	if (d == NULL)
		return -1;

	while ((di = readdir(d))) {
		if (di == NULL || strcmp(di->d_name, ".") == 0 || strcmp(di->d_name, "..") == 0)
			continue;

		snprintf(path, PATH_MAX, "/dev/input/%s", di->d_name);
		fd = open(path, O_RDONLY);
		if (fd < 0)
			continue;

		rc = ioctl(fd, EVIOCGNAME(sizeof(input_name) - 1), &input_name);
		if (rc < 0)
			continue;

		c = strstr((char *) &input_name, "\n");
		if (c != NULL)
			*c = '\0';

		if (strcmp(input_name, name) == 0)
			return fd;
		else
			close(fd);
	}

	return -1;
}

int sysfs_path_prefix(char *name, char *path_prefix)
{
	DIR *d;
	struct dirent *di;

	char input_name[80] = { 0 };
	char path[PATH_MAX];
	char *c;
	int fd;

	if (name == NULL || path_prefix == NULL)
		return -EINVAL;

	d = opendir("/sys/class/input");
	if (d == NULL)
		return -1;

	while ((di = readdir(d))) {
		if (di == NULL || strcmp(di->d_name, ".") == 0 || strcmp(di->d_name, "..") == 0)
			continue;

		snprintf(path, PATH_MAX, "/sys/class/input/%s/name", di->d_name);

		fd = open(path, O_RDONLY);
		if (fd < 0)
			continue;

		read(fd, &input_name, sizeof(input_name));
		close(fd);

		c = strstr((char *) &input_name, "\n");
		if (c != NULL)
			*c = '\0';

		if (strcmp(input_name, name) == 0) {
			snprintf(path_prefix, PATH_MAX, "/sys/class/input/%s", di->d_name);
			return 0;
		}
	}

	return -1;
}

/*
 * Geomagneticd
 */

int offset_read(char *path, int *hard_offset, int *calib_offset, int *accuracy)
{
	char buf[100] = { 0 };
	int fd;
	int rc;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;

	rc = read(fd, &buf, sizeof(buf));
	close(fd);
	if (rc <= 0)
		return -1;

	rc = sscanf(buf, "%d %d %d %d %d %d %d",
		&hard_offset[0], &hard_offset[1], &hard_offset[2],
		&calib_offset[0], &calib_offset[1], &calib_offset[2], accuracy);

	if (rc != 7)
		return -1;

	return 0;
}

int offset_write(char *path, int *hard_offset, int *calib_offset, int accuracy)
{
	char buf[100] = { 0 };
	int fd;
	int rc;

	sprintf(buf, "%d %d %d %d %d %d %d\n",
		hard_offset[0], hard_offset[1], hard_offset[2],
		calib_offset[0], calib_offset[1], calib_offset[2], accuracy);

	fd = open(path, O_WRONLY);
	if (fd < 0)
		return -1;

	write(fd, buf, strlen(buf) + 1);
	close(fd);

	return 0;
}

int yas_cfg_read(int *hard_offset, int *calib_offset, int *accuracy)
{
	char buf[100] = { 0 };
	int fd;
	int rc;

	fd = open("/data/system/yas.cfg", O_RDONLY);
	if (fd < 0)
		return -1;

	rc = read(fd, &buf, sizeof(buf));
	close(fd);
	if (rc <= 0)
		return -1;

	rc = sscanf(buf, "%d,%d,%d,%d,%d,%d,%d",
		&hard_offset[0], &hard_offset[1], &hard_offset[2],
		&calib_offset[0], &calib_offset[1], &calib_offset[2], accuracy);

	if (rc != 7)
		return -1;

	return 0;
}

int yas_cfg_write(int *hard_offset, int *calib_offset, int accuracy)
{
	char buf[100] = { 0 };
	int fd;
	int rc;

	fd = open("/data/system/yas-backup.cfg", O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	if (fd < 0)
		return -1;

	sprintf(buf, "%d,%d,%d,%d,%d,%d,%d\n",
		hard_offset[0], hard_offset[1], hard_offset[2],
		calib_offset[0], calib_offset[1], calib_offset[2], accuracy);

	write(fd, buf, strlen(buf) + 1);
	close(fd);

	chmod("/data/system/yas-backup.cfg", 0644);
	rename("/data/system/yas-backup.cfg", "/data/system/yas.cfg");

	return 0;
}

int main(int argc, char *argv[])
{
	struct input_event event;

	char path[PATH_MAX] = { 0 };
	char path_offset[PATH_MAX] = { 0 };

	int offset_fd;
	int input_fd;

	int max_coeff[3] = { 40, 45, 45 };
	int hard_offset[3] = { 0 };
	int calib_offset[3] = { 0 };
	int accuracy = 0;

	int axis_min[3] = { 0 };
	int axis_max[3] = { 0 };
	int axis_calib[3] = { 0 };

	int x, y, z;

	int rc;
	int i;

	/*
	 * Wait for something to be ready and properly report the hard coeff.
	 * Without that, the hard coeff are reported to be around 127.
	 */

	ALOGD("Geomagneticd start");

	input_fd = input_open("geomagnetic_raw");
	if (input_fd < 0)
		goto sleep_loop;

	rc = sysfs_path_prefix("geomagnetic_raw", &path);
	if (rc < 0)
		goto sleep_loop;

	snprintf(path_offset, PATH_MAX, "%s/offsets", path);

	for (i=0 ; i < 3 ; i++) {
		axis_min[i] = 0;
		axis_max[i] = 0;
		calib_offset[i] = 0;
	}

	ALOGD("Reading config");

	rc = yas_cfg_read(&hard_offset, &calib_offset, &accuracy);
	if (rc == 0) {
		ALOGD("Setting initial offsets: %d %d %d, %d %d %d", hard_offset[0], hard_offset[1], hard_offset[2], calib_offset[0], calib_offset[1], calib_offset[2]);

		offset_write(path_offset, &hard_offset, &calib_offset, accuracy);

		for (i=0 ; i < 3 ; i++) {
			axis_min[i] = - calib_offset[i] - max_coeff[i] * 1000;
			axis_max[i] = calib_offset[i] + max_coeff[i] * 1000;
			axis_calib[i] = calib_offset[i];
		}
	} else {
		offset_read(path_offset, &hard_offset, &calib_offset, &accuracy);
		ALOGD("Reading initial offsets: %d %d %d", hard_offset[0], hard_offset[1], hard_offset[2]);

		for (i=0 ; i < 3 ; i++) {
			axis_min[i] = 0;
			axis_max[i] = 0;
			calib_offset[i] = 0;
		}
	}

loop:
	while (1) {
		read(input_fd, &event, sizeof(event));

		if (event.type == EV_SYN) {
			for (i=0 ; i < 3 ; i++) {
				if (-axis_min[i] < axis_max[i]) {
					axis_calib[i] = axis_max[i] - max_coeff[i] * 1000;
				} else {
					axis_calib[i] = axis_min[i] + max_coeff[i] * 1000;
				}

				axis_calib[i] = axis_calib[i] < 0 ? -axis_calib[i] : axis_calib[i];

				if (axis_calib[i] != calib_offset[i]) {
					calib_offset[i] = axis_calib[i];
					accuracy = 1;

					offset_write(path_offset, &hard_offset, &calib_offset, accuracy);
					yas_cfg_write(&hard_offset, &calib_offset, accuracy);
				}

//				printf("axis_calib[%d]=%d\n", i, axis_calib[i]);
			}

			if (hard_offset[0] == 127 && hard_offset[1] == 127 && hard_offset[2] == 127) {
				offset_read(path_offset, &hard_offset, &calib_offset, &accuracy);

				if (hard_offset[0] != 127 || hard_offset[1] != 127 || hard_offset[2] != 127) {
					ALOGD("Reading offsets: %d %d %d", hard_offset[0], hard_offset[1], hard_offset[2]);
					yas_cfg_write(&hard_offset, &calib_offset, accuracy);
				}
			}
		}

		if(event.type == EV_ABS) {
			switch (event.code) {
				case ABS_X:
					x = event.value;
					if (x < axis_min[0])
						axis_min[0] = x;
					if (x > axis_max[0])
						axis_max[0] = x;
					break;
				case ABS_Y:
					y = event.value;
					if (y < axis_min[1])
						axis_min[1] = y;
					if (y > axis_max[1])
						axis_max[1] = y;
					break;
				case ABS_Z:
					z = event.value;
					if (z < axis_min[2])
						axis_min[2] = z;
					if (z > axis_max[2])
						axis_max[2] = z;
					break;
			}
		}
	}

sleep_loop:
	while (1) {
		sleep(3600);
	}

	return 0;
}
