/*
 * vector.c
 *
 *  Created on: 27/04/2023
 *      Author: paulo
 */

#include <datastructure/vector.h>

bool vector_init(struct vector *vec)
{
	/* sanity checks */
	if (!vec)
		return false;

	vec->data = zalloc(VECTOR_DEFAULT_INITAL_SIZE);

	if (!vec->data)
		return false;

	vec->capacity = VECTOR_DEFAULT_INITAL_SIZE;
	vec->len = 0;

	return true;
}

bool vector_push(struct vector *vec, void *data, size_t len)
{
	size_t new_cap;

	/* sanity checks */
	if (!vec || !data || len == 0)
		return false;

	/* does the vector need to grow? */
	if (vec->len + len > vec->capacity / 2) {
		new_cap = (vec->len + len) * 3;
		vec->data = realloc(vec->data, new_cap);

		if (!vec->data)
			return false;

		vec->capacity = new_cap;
	}

	/* performs the copy */
	memcpy(vec->data + vec->len, data, len);
	vec->len += len;

	return true;
}

void vector_free(struct vector *vec)
{
	/* sanity checks */
	BUG_ON(!vec);

	vec->capacity = 0;
	vec->len = 0;
	free(vec->data);
	vec->data = NULL;
}
