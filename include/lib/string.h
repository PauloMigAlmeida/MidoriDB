/*
 * string.h
 *
 *  Created on: 2/06/2023
 *      Author: paulo
 */

#ifndef INCLUDE_LIB_STRING_H_
#define INCLUDE_LIB_STRING_H_

#include <compiler/common.h>

bool strstarts(const char *str, const char *prefix);

void strrand(char *str, size_t len);

#endif /* INCLUDE_LIB_STRING_H_ */
