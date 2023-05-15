/*
 * syntax.h
 *
 *  Created on: 26/04/2023
 *      Author: paulo
 */

#ifndef INCLUDE_PARSER_SYNTAX_H_
#define INCLUDE_PARSER_SYNTAX_H_

#include <compiler/common.h>
#include <datastructure/stack.h>
#include <parser/ast.h>

int syntax_parse(char *stmt, struct stack *out);

#endif /* INCLUDE_PARSER_SYNTAX_H_ */
