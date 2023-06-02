/*
 * regex.h
 *
 *  Created on: 2/06/2023
 *      Author: paulo
 */

#ifndef INCLUDE_LIB_REGEX_H_
#define INCLUDE_LIB_REGEX_H_

#include <compiler/common.h>
#include <datastructure/stack.h>

bool __must_check regex_ext_match_grp(const char *text, const char *exp, struct stack *out);

#endif /* INCLUDE_LIB_REGEX_H_ */
