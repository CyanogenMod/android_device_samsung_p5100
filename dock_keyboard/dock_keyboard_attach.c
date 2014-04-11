/*
 * Copyright (C) 2012, Samsung Electronics
 * Author : Heetae Ahn <heetae82.ahn@samsung.com>
 *
 * Based on inputattach.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/serio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#define SERIO_SAMSUNG		0x3d
#define CONSOLE_PROC		"/proc/consoles"
#define MAX_DEV_NAME_SIZE	64
#define MAX_PATH_SIZE		128
#define MAX_BUF_SIZE		255
#define DELIM			"/"

void setline(int fd, int flags, int speed)
{
	struct termios t;

	tcgetattr(fd, &t);

	t.c_cflag = flags | CREAD | HUPCL | CLOCAL;
	t.c_iflag = IGNBRK | IGNPAR;
	t.c_oflag = 0;
	t.c_lflag = 0;
	t.c_cc[VMIN] = 1;
	t.c_cc[VTIME] = 0;

	cfsetispeed(&t, speed);
	cfsetospeed(&t, speed);

	tcsetattr(fd, TCSANOW, &t);
}

int main(int argc, char **argv)
{

	unsigned long devt;
	int ldisc;
	int type;
	long id, extra;
	int fd;
	char c;
	FILE *fp;
	char uart_name[MAX_DEV_NAME_SIZE];
	char uart_path[MAX_PATH_SIZE];
	char buf[MAX_BUF_SIZE];
	char *token;
	char *ptr;

	if (argc < 1 || argc > 2)
		return 1;

	strncpy(uart_path, argv[1], MAX_PATH_SIZE);
	token = strtok_r(argv[1], DELIM, &ptr);

	while (token) {
		strncpy(uart_name, token, MAX_DEV_NAME_SIZE);
		token = strtok_r(NULL, DELIM, &ptr);
	}

	if (!(fp = fopen(CONSOLE_PROC, "r"))) {
		fprintf(stderr,
			"dock_kbd_attach: can't open console proc file\n");
		return 1;
	}

	while (fgets(buf, MAX_BUF_SIZE, fp)) {
		if (!strncmp(uart_name, buf, strlen(uart_name))) {
			fprintf(stderr,
				"dock_kbd_attach: UART %s is used by console\n",
				uart_name);
			fclose(fp);
			return 1;
		}
	};

	fprintf(stdout, "dock_kbd_attach: UART %s is not used by console\n",
		uart_name);

	fclose(fp);

	if ((fd = open(uart_path, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0) {
		fprintf(stderr, "dock_kbd_attach: can't open UART device\n");
		return 1;
	}

	setline(fd, CS8, B9600);
	id = 0;
	extra = 0;
	ldisc = N_MOUSE;

	if (ioctl(fd, TIOCSETD, &ldisc)) {
		fprintf(stderr, "dock_kbd_attach: can't set line discipline\n");
		close(fd);
		return 1;
	}

	devt = SERIO_SAMSUNG | (id << 8) | (extra << 16);

	if (ioctl(fd, SPIOCSTYPE, &devt)) {
		fprintf(stderr, "dock_kbd_attach: can't set device type\n");
		close(fd);
		return 1;
	}

	while (1)
		read(fd, NULL, 0);

	ldisc = 0;
	ioctl(fd, TIOCSETD, &ldisc);
	close(fd);

	return 0;
}
