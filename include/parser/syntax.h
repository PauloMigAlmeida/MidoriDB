/*
 * syntax.h
 *
 *  Created on: 26/04/2023
 *      Author: paulo
 */

#ifndef INCLUDE_PARSER_SYNTAX_H_
#define INCLUDE_PARSER_SYNTAX_H_

#include <compiler/common.h>
#include <datastructure/vector.h>

extern int bison_parse_string(const char* in, struct vector *out);

int syntax_parse(char *stmt);

#endif /* INCLUDE_PARSER_SYNTAX_H_ */
