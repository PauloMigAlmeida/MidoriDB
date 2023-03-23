/*
 * bit.h
 *
 *  Created on: Jul 5, 2021
 *      Author: Paulo Almeida
 */

#ifndef INCLUDE_COMPILER_BIT_H_
#define INCLUDE_COMPILER_BIT_H_

#include <limits.h>
#include <math.h>
#include "compiler/compiler_attributes.h"


static __force_inline bool test_bit(void* field, uint8_t bit_number) {
    size_t byte_no = (size_t)floorl(bit_number / CHAR_BIT);
    bit_number -= byte_no * CHAR_BIT;
    char* field_ptr = (char*)field;
    return (field_ptr[byte_no] >> bit_number) & 1ULL;
}

static __force_inline void set_bit(void* field, uint8_t bit_number) {
    size_t byte_no = (size_t)floorl(bit_number / CHAR_BIT);
    bit_number -= byte_no * CHAR_BIT;
    char* field_ptr = (char*)field;
    field_ptr[byte_no] = field_ptr[byte_no] | 1ULL << bit_number;
}

static __force_inline void clear_bit(void* field, uint8_t bit_number) {
    size_t byte_no = (size_t)floorl(bit_number / CHAR_BIT);
    bit_number -= byte_no * CHAR_BIT;
    char* field_ptr = (char*)field;
    field_ptr[byte_no] = field_ptr[byte_no] & ~(1UL << bit_number);
}

static __force_inline void toggle_bit(void* field, uint8_t bit_number) {
    size_t byte_no = (size_t)floorl(bit_number / CHAR_BIT);
    bit_number -= byte_no * CHAR_BIT;
    char* field_ptr = (char*)field;
    field_ptr[byte_no] = field_ptr[byte_no] ^ (1UL << bit_number);
}

#endif /* INCLUDE_COMPILER_BIT_H_ */
