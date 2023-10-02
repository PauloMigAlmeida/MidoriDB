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

static void select_case_2(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_sel_select_node *select_node;
	struct ast_sel_alias_node *alias_node;
	struct ast_sel_exprval_node *val_node;
	struct list_head *pos1, *pos2;

	parse_stmt("SELECT 123 as result;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	select_node = (typeof(select_node))root;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT(list_is_empty(&select_node->head));
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 1);

	list_for_each(pos1, select_node->node_children_head)
	{
		alias_node = list_entry(pos1, typeof(*alias_node), head);
		CU_ASSERT_EQUAL(alias_node->node_type, AST_TYPE_SEL_ALIAS);
		CU_ASSERT_STRING_EQUAL(alias_node->alias_value, "result");
		CU_ASSERT_FALSE(list_is_empty(&alias_node->head));
		CU_ASSERT_EQUAL(list_length(alias_node->node_children_head), 1);

		list_for_each(pos2, alias_node->node_children_head)
		{
			val_node = list_entry(pos2, typeof(*val_node), head);
			CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_SEL_EXPRVAL);
			CU_ASSERT(val_node->value_type.is_intnum);
			CU_ASSERT_EQUAL(val_node->int_val, 123);
			CU_ASSERT_FALSE(list_is_empty(&val_node->head));
			CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);
		}
	}

	queue_free(&ct);
	ast_free(root);
}

static void select_case_3(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_sel_select_node *select_node;
	struct ast_sel_exprop_node *op_node_1, *op_node_2;
	struct ast_sel_exprval_node *val_node_1, *val_node_2;
	struct list_head *pos1, *pos2, *pos3;
	int i = 0, j = 0;

	parse_stmt("SELECT (2 + 3) * 2;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	select_node = (typeof(select_node))root;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT(list_is_empty(&select_node->head));
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 1);

	list_for_each(pos1, select_node->node_children_head)
	{
		op_node_1 = list_entry(pos1, typeof(*op_node_1), head);
		CU_ASSERT_EQUAL(op_node_1->node_type, AST_TYPE_SEL_EXPROP);
		CU_ASSERT_EQUAL(op_node_1->op_type, AST_SEL_EXPR_OP_MUL);
		CU_ASSERT_FALSE(list_is_empty(&op_node_1->head));
		CU_ASSERT_EQUAL(list_length(op_node_1->node_children_head), 2);

		list_for_each(pos2, op_node_1->node_children_head)
		{
			if (i == 0) {
				val_node_1 = list_entry(pos2, typeof(*val_node_1), head);
				CU_ASSERT_EQUAL(val_node_1->node_type, AST_TYPE_SEL_EXPRVAL);
				CU_ASSERT(val_node_1->value_type.is_intnum);
				CU_ASSERT_EQUAL(val_node_1->int_val, 2);
				CU_ASSERT_FALSE(list_is_empty(&val_node_1->head));
				CU_ASSERT_EQUAL(list_length(val_node_1->node_children_head), 0);
			} else {
				op_node_2 = list_entry(pos2, typeof(*op_node_2), head);
				CU_ASSERT_EQUAL(op_node_2->node_type, AST_TYPE_SEL_EXPROP);
				CU_ASSERT_EQUAL(op_node_2->op_type, AST_SEL_EXPR_OP_ADD);
				CU_ASSERT_FALSE(list_is_empty(&op_node_2->head));
				CU_ASSERT_EQUAL(list_length(op_node_2->node_children_head), 2);

				list_for_each(pos3, op_node_2->node_children_head)
				{
					val_node_2 = list_entry(pos3, typeof(*val_node_2), head);
					CU_ASSERT_EQUAL(val_node_2->node_type, AST_TYPE_SEL_EXPRVAL);
					CU_ASSERT(val_node_2->value_type.is_intnum);
					CU_ASSERT_FALSE(list_is_empty(&val_node_2->head));
					CU_ASSERT_EQUAL(list_length(val_node_2->node_children_head), 0);

					if (j == 0) {
						CU_ASSERT_EQUAL(val_node_2->int_val, 3);
					} else {
						CU_ASSERT_EQUAL(val_node_2->int_val, 2);
					}
					j++;

				}
			}
			i++;
		}

	}

	queue_free(&ct);
	ast_free(root);
}

static void select_case_4(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_sel_select_node *select_node;
	struct ast_sel_selectall_node *all_node;
	struct ast_sel_table_node *table_node;
	struct list_head *pos1;
	int i = 0;

	parse_stmt("SELECT * FROM A;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	select_node = (typeof(select_node))root;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT(list_is_empty(&select_node->head));
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 2);

	list_for_each(pos1, select_node->node_children_head)
	{
		if (i == 0) {
			all_node = list_entry(pos1, typeof(*all_node), head);
			CU_ASSERT_EQUAL(all_node->node_type, AST_TYPE_SEL_SELECTALL);
			CU_ASSERT_FALSE(list_is_empty(&all_node->head));
			CU_ASSERT_EQUAL(list_length(all_node->node_children_head), 0);
		} else {
			table_node = list_entry(pos1, typeof(*table_node), head);
			CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
			CU_ASSERT_STRING_EQUAL(table_node->table_name, "A");
			CU_ASSERT_FALSE(list_is_empty(&table_node->head));
			CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);
		}
		i++;

	}

	queue_free(&ct);
	ast_free(root);
}

void test_ast_build_tree_select(void)
{
	/* SELECTNODATA */
	select_case_1();
	/* SELECTNODATA + alias */
	select_case_2();
	/* SELECTNODATA + recursive expression */
	select_case_3();
	/* ALL + single table + no where-clause */
	select_case_4();
}
