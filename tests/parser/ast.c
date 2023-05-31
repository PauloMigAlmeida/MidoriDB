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
		printf("stack pos: %d, content: %s\n", i, ptr);
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

static void ast_free(struct ast_node *root)
{
	struct list_head *pos = NULL;
	struct list_head *tmp_pos = NULL;
	struct ast_node *entry = NULL;

	if (root->node_children_head) {

		list_for_each_safe(pos,tmp_pos, root->node_children_head)
		{
			entry = list_entry(pos, typeof(*entry), head);
			ast_free(entry);
		}

		free(root->node_children_head);
	}

	free(root);
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
	 * - tweak ast_build_tree so it can parse multiple STMTs in the future.
	 * - make ast_build_tree routine better... this looks hacky as fuck
	 * - fix all the __must_check warnings... I've been sloppy lately with that
	 */

	stack_free(&st);
	ast_free(root);
}
