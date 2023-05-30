/*
 * ast.c
 *
 *  Created on: 15/05/2023
 *      Author: paulo
 */

#include <tests/parser.h>
#include <parser/syntax.h>
#include <parser/ast.h>

static void print_stack_content(struct stack *st)
{
	for (int i = st->idx; i >= 0; i--) {
		char *ptr = *((char**)(st->arr->data + i * sizeof(uintptr_t)));
		printf("%d: %s\n", i, ptr);
	}
	printf("\n");
}

static void build_stack(char *stmt, struct stack *out)
{
	CU_ASSERT(stack_init(out));
	CU_ASSERT_EQUAL(syntax_parse(stmt, out), 0);
	printf("\n\n");
	print_stack_content(out);
}

void test_ast_build_tree(void)
{
	struct stack st = {0};
	struct ast_node *root;

	build_stack("CREATE TABLE IF NOT EXISTS A ("
			"f1 INTEGER PRIMARY KEY AUTO_INCREMENT,"
			"f2 INT UNIQUE,"
			"f3 DOUBLE NOT NULL);",
			&st);

	root = ast_build_tree(&st);
	CU_ASSERT_PTR_NOT_NULL(root);

	/**
	 *
	 * Parser -> Syntax -> Semantic -> Optimiser -> Execution Plan (This works for Select, Insert, Update, Delete)
	 * Parser -> Syntax -> Semantic -> Execution Plan (this works for CREATE, DROP?)
	 *
	 * Things to fix: 30/05/2023
	 *
	 * - root node is returned however we don't have a recursive free routine yet - so mem leak
	 * - tweak ast_build_tree so it can parse multiple STMTs in the future.
	 * - make ast_build_tree routine better... this looks hacky as fuck
	 * - fix all the __must_check warnings... I've been sloppy lately with that
	 */

	stack_free(&st);
}
