/*
 * bit.h
 *
 *  Created on: Mar 24, 2023
 *      Author: Paulo Almeida
 */

#ifndef INCLUDE_COMPILER_BIT_H_
#define INCLUDE_COMPILER_BIT_H_

#include "compiler/common.h"

bool bit_test(void* field, size_t bit_number, size_t len);

void bit_set(void* field, uint8_t bit_number, size_t len);

void bit_clear(void* field, uint8_t bit_number, size_t len);

void bit_toggle(void* field, uint8_t bit_number, size_t len);

#endif /* INCLUDE_COMPILER_BIT_H_ */
