/*
 * ast.c
 *
 *  Created on: 15/05/2023
 *      Author: paulo
 */

#include <tests/parser.h>
#include <parser/syntax.h>
#include <parser/ast.h>

static void build_stack(char *stmt, struct stack *out)
{
	CU_ASSERT(stack_init(out));
	CU_ASSERT_EQUAL(syntax_parse(stmt, out), 0);
}

void test_ast_build_tree(void)
{
	struct stack st = {0};
	struct ast_node *root;

	build_stack("CREATE TABLE IF NOT EXISTS A ("
			"f1 INTEGER PRIMARY KEY AUTO_INCREMENT,"
			"f2 INT UNIQUE,"
			"f3 DOUBLE NOT NULL,"
			"f4 FLOAT NULL);", &st);

	root = ast_build_tree(&st);
	CU_ASSERT_PTR_NOT_NULL(root);

	stack_free(&st);
}
