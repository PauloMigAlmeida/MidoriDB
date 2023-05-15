/*
 * parser.h
 *
 *  Created on: 26/04/2023
 *      Author: paulo
 */

#ifndef INCLUDE_TESTS_PARSER_H_
#define INCLUDE_TESTS_PARSER_H_

#include <tests/unittest.h>

/* test suites */
bool parser_init_suite(void);

/* test cases */
void test_syntax_parse(void);
void test_ast_build_tree(void);

#endif /* INCLUDE_TESTS_PARSER_H_ */
