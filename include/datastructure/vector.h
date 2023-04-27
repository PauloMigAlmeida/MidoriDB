/*
 * vector.h
 *
 *  Created on: 27/04/2023
 *      Author: paulo
 */

#ifndef INCLUDE_DATASTRUCTURE_VECTOR_H_
#define INCLUDE_DATASTRUCTURE_VECTOR_H_

#include "compiler/common.h"

#define VECTOR_DEFAULT_INITAL_SIZE	16

/*
 * Simplistic dynamic array data structure
 */
struct vector {
	char *data;
	size_t len; /* number of chars used in vector */
	size_t capacity; /* current total capacity */
};

/**
 * vector_init - initialise vector
 * @vec: vector to be initialised
 *
 * this function returns true vector could be initialised, false otherwise
 */
bool vector_init(struct vector *vec);

/**
 * vector_push - push content into vector
 * @vec: vector reference
 * @data: data to be pushed
 * @len: amount of data to be copied from data ptr
 *
 * this function returns true data could be inserted into vector, false otherwise
 */
bool vector_push(struct vector *vec, void *data, size_t len);

/**
 * vector_free - free vector content
 * @vec: vector reference
 */
void vector_free(struct vector *vec);

/**
 * vector_clear - clear vector content
 * @vec: vector reference
 */
void vector_clear(struct vector *vec);

#endif /* INCLUDE_DATASTRUCTURE_VECTOR_H_ */
