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

#include <linux/input.h>

#ifndef _ORIENTATION_H_
#define _ORIENTATION_H_

/*
 * Vector
 */

struct vector {
	float x;
	float y;
	float z;

};

typedef struct vector vector;

void vector_add(vector *v, vector *a);
void vector_multiply(vector *v, float k);
void vector_cross(vector *v, vector *c, vector *out);
float vector_scalar(vector *v, vector *d);
float vector_length(vector *v);

/*
 * Input
 */

int64_t input_timestamp(struct input_event *event);
int input_open(char *name, int write);
int sysfs_path_prefix(char *name, char *path_prefix);

#endif
