/*
 * parser.h
 *
 *  Created on: 26/04/2023
 *      Author: paulo
 */

#ifndef INCLUDE_TESTS_PARSER_H_
#define INCLUDE_TESTS_PARSER_H_

#include <tests/unittest.h>
#include <datastructure/queue.h>
#include <parser/ast.h>

/* test suites */
bool parser_init_suite(void);

/* test cases */
void test_syntax_parse(void);
void test_ast_build_tree(void);
void test_semantic_analyze(void);

/* utility functions which are shared among tests */
void print_queue(struct queue *ct);
void parse_stmt(char *stmt, struct queue *out);
struct ast_node* build_ast(char *stmt);

#endif /* INCLUDE_TESTS_PARSER_H_ */
