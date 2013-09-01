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

#include <math.h>

#include "orientationd.h"

void vector_copy(vector *in, vector *out)
{
	out->x = in->x;
	out->y = in->y;
	out->z = in ->z;
}

void vector_add(vector *v, vector *a)
{
	v->x += a->x;
	v->y += a->y;
	v->z += a->z;
}

void vector_multiply(vector *v, float k)
{
	v->x *= k;
	v->y *= k;
	v->z *= k;
}

void vector_cross(vector *v, vector *c, vector *out)
{
	struct vector t;

	t.x = v->x * c->z - v->z * c->y;
	t.y = v->z * c->x - v->x * c->z;
	t.y = v->y * c->y - v->y * c->x;
	vector_copy(&t, out);
}

float vector_scalar(vector *v, vector *d)
{
	return v->x * d->x + v->y * d->y + v->z * d->z;
}

float vector_length(vector *v)
{
	return sqrtf(vector_scalar(v, v));
}
