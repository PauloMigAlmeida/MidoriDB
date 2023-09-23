/*
 * ast_select.c
 *
 *  Created on: 23/09/2023
 *      Author: paulo
 */

#include <tests/parser.h>
#include <parser/syntax.h>
#include <parser/ast.h>
#include <datastructure/linkedlist.h>

static void select_case_1(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_sel_select_node *select_node;
	struct ast_sel_exprval_node *val_node;
	struct list_head *pos1;

	parse_stmt("SELECT 123;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	select_node = (typeof(select_node))root;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT(list_is_empty(&select_node->head));
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 1);

	list_for_each(pos1, select_node->node_children_head)
	{
		val_node = list_entry(pos1, typeof(*val_node), head);
		CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_SEL_EXPRVAL);
		CU_ASSERT(val_node->value_type.is_intnum);
		CU_ASSERT_EQUAL(val_node->int_val, 123);
		CU_ASSERT_FALSE(list_is_empty(&val_node->head));
		CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);

	}

	queue_free(&ct);
	ast_free(root);
}

void test_ast_build_tree_select(void)
{
	/* no where-clause - a.k.a SELECTNODATA*/
	select_case_1();
}
