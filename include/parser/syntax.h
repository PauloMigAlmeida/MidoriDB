/*
 * syntax.h
 *
 *  Created on: 26/04/2023
 *      Author: paulo
 */

#ifndef INCLUDE_PARSER_SYNTAX_H_
#define INCLUDE_PARSER_SYNTAX_H_

#include <compiler/common.h>

extern int bison_parse_string(const char* in);

int syntax_parse(char *stmt);

#endif /* INCLUDE_PARSER_SYNTAX_H_ */
