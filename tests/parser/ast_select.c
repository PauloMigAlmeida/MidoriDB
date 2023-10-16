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

static void select_case_5(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_sel_select_node *select_node;
	struct ast_sel_selectall_node *all_node;
	struct ast_sel_table_node *table_node;
	struct list_head *pos1;
	int i = 0;

	parse_stmt("SELECT DISTINCT * FROM A;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	select_node = (typeof(select_node))root;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT(select_node->distinct);
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

static void select_case_6(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_sel_select_node *select_node;
	struct ast_sel_exprval_node *field_node;
	struct ast_sel_table_node *table_node;
	struct list_head *pos1;
	int i = 0;

	parse_stmt("SELECT f1,f2 FROM A;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	select_node = (typeof(select_node))root;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT(list_is_empty(&select_node->head));
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 3);

	list_for_each(pos1, select_node->node_children_head)
	{
		if (i == 0) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_EXPRVAL);
			CU_ASSERT_STRING_EQUAL(field_node->name_val, "f1");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
		} else if (i == 1) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_EXPRVAL);
			CU_ASSERT_STRING_EQUAL(field_node->name_val, "f2");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
		}
		else {
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

static void select_case_7(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_sel_select_node *select_node;
	struct ast_sel_exprval_node *field_node, *val_node;
	struct ast_sel_limit_node *limit_node;
	struct ast_sel_table_node *table_node;
	struct list_head *pos1, *pos2;
	int i = 0, j = 0;

	parse_stmt("SELECT f1,f2 FROM A LIMIT 1,5;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	select_node = (typeof(select_node))root;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT(list_is_empty(&select_node->head));
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 4);

	list_for_each(pos1, select_node->node_children_head)
	{
		if (i == 0) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_EXPRVAL);
			CU_ASSERT_STRING_EQUAL(field_node->name_val, "f1");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
		} else if (i == 1) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_EXPRVAL);
			CU_ASSERT_STRING_EQUAL(field_node->name_val, "f2");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
		} else if (i == 2) {
			table_node = list_entry(pos1, typeof(*table_node), head);
			CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
			CU_ASSERT_STRING_EQUAL(table_node->table_name, "A");
			CU_ASSERT_FALSE(list_is_empty(&table_node->head));
			CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);
		} else {
			limit_node = list_entry(pos1, typeof(*limit_node), head);
			CU_ASSERT_EQUAL(limit_node->node_type, AST_TYPE_SEL_LIMIT);
			CU_ASSERT_FALSE(list_is_empty(&limit_node->head));
			CU_ASSERT_EQUAL(list_length(limit_node->node_children_head), 2);

			list_for_each(pos2, limit_node->node_children_head)
			{
				val_node = list_entry(pos2, typeof(*val_node), head);
				CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_SEL_EXPRVAL);
				CU_ASSERT(val_node->value_type.is_intnum);
				CU_ASSERT_FALSE(list_is_empty(&val_node->head));
				CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);

				if (j == 0) {
					CU_ASSERT_EQUAL(val_node->int_val, 1);
				} else {
					CU_ASSERT_EQUAL(val_node->int_val, 5);
				}
				j++;
			}

		}
		i++;

	}

	queue_free(&ct);
	ast_free(root);
}

static void select_case_8(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_sel_select_node *select_node;
	struct ast_sel_exprval_node *field_node;
	struct ast_sel_alias_node *alias_node;
	struct ast_sel_table_node *table_node;
	struct list_head *pos1, *pos2;
	int i = 0;

	parse_stmt("SELECT f1 as v1,f2 FROM A;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	select_node = (typeof(select_node))root;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT(list_is_empty(&select_node->head));
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 3);

	list_for_each(pos1, select_node->node_children_head)
	{
		if (i == 0) {
			alias_node = list_entry(pos1, typeof(*alias_node), head);
			CU_ASSERT_EQUAL(alias_node->node_type, AST_TYPE_SEL_ALIAS);
			CU_ASSERT_STRING_EQUAL(alias_node->alias_value, "v1");
			CU_ASSERT_FALSE(list_is_empty(&alias_node->head));
			CU_ASSERT_EQUAL(list_length(alias_node->node_children_head), 1);

			list_for_each(pos2, alias_node->node_children_head)
			{
				field_node = list_entry(pos2, typeof(*field_node), head);
				CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_EXPRVAL);
				CU_ASSERT_STRING_EQUAL(field_node->name_val, "f1");
				CU_ASSERT_FALSE(list_is_empty(&field_node->head));
				CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
			}
		} else if (i == 1) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_EXPRVAL);
			CU_ASSERT_STRING_EQUAL(field_node->name_val, "f2");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
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

static void select_case_9(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_sel_select_node *select_node;
	struct ast_sel_exprval_node *field_node;
	struct ast_sel_table_node *table_node;
	struct list_head *pos1;
	int i = 0;

	parse_stmt("SELECT f1, f2 FROM A, B;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	select_node = (typeof(select_node))root;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT(list_is_empty(&select_node->head));
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 4);

	list_for_each(pos1, select_node->node_children_head)
	{
		if (i == 0) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_EXPRVAL);
			CU_ASSERT_STRING_EQUAL(field_node->name_val, "f1");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
		} else if (i == 1) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_EXPRVAL);
			CU_ASSERT_STRING_EQUAL(field_node->name_val, "f2");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
		} else if (i == 2) {
			table_node = list_entry(pos1, typeof(*table_node), head);
			CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
			CU_ASSERT_STRING_EQUAL(table_node->table_name, "A");
			CU_ASSERT_FALSE(list_is_empty(&table_node->head));
			CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);
		} else {
			table_node = list_entry(pos1, typeof(*table_node), head);
			CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
			CU_ASSERT_STRING_EQUAL(table_node->table_name, "B");
			CU_ASSERT_FALSE(list_is_empty(&table_node->head));
			CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);
		}
		i++;

	}

	queue_free(&ct);
	ast_free(root);
}

static void select_case_10(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_sel_select_node *select_node;
	struct ast_sel_fieldname_node *field_node;
	struct ast_sel_table_node *table_node;
	struct ast_sel_alias_node *alias_node;
	struct list_head *pos1, *pos2;
	int i = 0;

	parse_stmt("SELECT A1.f1, B.f2 FROM A as A1, B;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	select_node = (typeof(select_node))root;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT(list_is_empty(&select_node->head));
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 4);

	list_for_each(pos1, select_node->node_children_head)
	{
		if (i == 0) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
			CU_ASSERT_STRING_EQUAL(field_node->table_name, "A1");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
		} else if (i == 1) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "f2");
			CU_ASSERT_STRING_EQUAL(field_node->table_name, "B");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
		} else if (i == 2) {
			alias_node = list_entry(pos1, typeof(*alias_node), head);
			CU_ASSERT_EQUAL(alias_node->node_type, AST_TYPE_SEL_ALIAS);
			CU_ASSERT_STRING_EQUAL(alias_node->alias_value, "A1");
			CU_ASSERT_FALSE(list_is_empty(&alias_node->head));
			CU_ASSERT_EQUAL(list_length(alias_node->node_children_head), 1);

			list_for_each(pos2, alias_node->node_children_head)
			{
				table_node = list_entry(pos2, typeof(*table_node), head);
				CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
				CU_ASSERT_STRING_EQUAL(table_node->table_name, "A");
				CU_ASSERT_FALSE(list_is_empty(&table_node->head));
				CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);
			}
		} else {
			table_node = list_entry(pos1, typeof(*table_node), head);
			CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
			CU_ASSERT_STRING_EQUAL(table_node->table_name, "B");
			CU_ASSERT_FALSE(list_is_empty(&table_node->head));
			CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);
		}
		i++;

	}

	queue_free(&ct);
	ast_free(root);
}

static void select_case_11(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_sel_select_node *select_node;
	struct ast_sel_fieldname_node *field_node;
	struct ast_sel_table_node *table_node;
	struct list_head *pos1;
	int i = 0;

	parse_stmt("SELECT A.f1, B.f2 FROM A, B;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	select_node = (typeof(select_node))root;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT(list_is_empty(&select_node->head));
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 4);

	list_for_each(pos1, select_node->node_children_head)
	{
		if (i == 0) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
			CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
		} else if (i == 1) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "f2");
			CU_ASSERT_STRING_EQUAL(field_node->table_name, "B");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
		} else if (i == 2) {
			table_node = list_entry(pos1, typeof(*table_node), head);
			CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
			CU_ASSERT_STRING_EQUAL(table_node->table_name, "A");
			CU_ASSERT_FALSE(list_is_empty(&table_node->head));
			CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);
		} else {
			table_node = list_entry(pos1, typeof(*table_node), head);
			CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
			CU_ASSERT_STRING_EQUAL(table_node->table_name, "B");
			CU_ASSERT_FALSE(list_is_empty(&table_node->head));
			CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);
		}
		i++;

	}

	queue_free(&ct);
	ast_free(root);
}

static void select_case_12(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_sel_select_node *select_node;
	struct ast_sel_fieldname_node *field_node;
	struct ast_sel_table_node *table_node;
	struct ast_sel_where_node *where_node;
	struct ast_sel_cmp_node *cmp_node;
	struct list_head *pos1, *pos2, *pos3;
	int i = 0, j = 0;

	parse_stmt("SELECT A.f1, B.f2 FROM A, B WHERE A.f1 = B.f2;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	select_node = (typeof(select_node))root;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT(list_is_empty(&select_node->head));
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 5);

	list_for_each(pos1, select_node->node_children_head)
	{
		if (i == 0) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
			CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
		} else if (i == 1) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "f2");
			CU_ASSERT_STRING_EQUAL(field_node->table_name, "B");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
		} else if (i == 2) {
			table_node = list_entry(pos1, typeof(*table_node), head);
			CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
			CU_ASSERT_STRING_EQUAL(table_node->table_name, "A");
			CU_ASSERT_FALSE(list_is_empty(&table_node->head));
			CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);
		} else if (i == 3) {
			table_node = list_entry(pos1, typeof(*table_node), head);
			CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
			CU_ASSERT_STRING_EQUAL(table_node->table_name, "B");
			CU_ASSERT_FALSE(list_is_empty(&table_node->head));
			CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);
		} else {
			where_node = list_entry(pos1, typeof(*where_node), head);
			CU_ASSERT_EQUAL(where_node->node_type, AST_TYPE_SEL_WHERE);
			CU_ASSERT_FALSE(list_is_empty(&where_node->head));
			CU_ASSERT_EQUAL(list_length(where_node->node_children_head), 1);

			list_for_each(pos2, where_node->node_children_head)
			{
				cmp_node = list_entry(pos2, typeof(*cmp_node), head);
				CU_ASSERT_EQUAL(cmp_node->node_type, AST_TYPE_SEL_CMP);
				CU_ASSERT_EQUAL(cmp_node->cmp_type, AST_CMP_EQUALS_OP);
				CU_ASSERT_FALSE(list_is_empty(&cmp_node->head));
				CU_ASSERT_EQUAL(list_length(cmp_node->node_children_head), 2);

				list_for_each(pos3, cmp_node->node_children_head)
				{
					if (j == 0) {
						field_node = list_entry(pos3, typeof(*field_node), head);
						CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
						CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
						CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
						CU_ASSERT_FALSE(list_is_empty(&field_node->head));
						CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
					} else {
						field_node = list_entry(pos3, typeof(*field_node), head);
						CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
						CU_ASSERT_STRING_EQUAL(field_node->col_name, "f2");
						CU_ASSERT_STRING_EQUAL(field_node->table_name, "B");
						CU_ASSERT_FALSE(list_is_empty(&field_node->head));
						CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
					}
					j++;
				}
			}
		}
		i++;

	}

	queue_free(&ct);
	ast_free(root);
}

static void select_case_13(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_sel_select_node *select_node;
	struct ast_sel_fieldname_node *field_node;
	struct ast_sel_table_node *table_node;
	struct ast_sel_join_node *join_node;
	struct ast_sel_onexpr_node *onexpr_node;
	struct ast_sel_cmp_node *cmp_node;
	struct list_head *pos1, *pos2, *pos3, *pos4;
	int i = 0, j = 0, k = 0;

	parse_stmt("SELECT A.f1, B.f2 FROM A JOIN B ON A.f1 = B.f2;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	select_node = (typeof(select_node))root;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT(list_is_empty(&select_node->head));
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 3);

	list_for_each(pos1, select_node->node_children_head)
	{
		if (i == 0) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
			CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
		} else if (i == 1) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "f2");
			CU_ASSERT_STRING_EQUAL(field_node->table_name, "B");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
		} else {
			join_node = list_entry(pos1, typeof(*join_node), head);
			CU_ASSERT_EQUAL(join_node->node_type, AST_TYPE_SEL_JOIN);
			CU_ASSERT_EQUAL(join_node->join_type, AST_SEL_JOIN_INNER);
			CU_ASSERT_FALSE(list_is_empty(&join_node->head));
			CU_ASSERT_EQUAL(list_length(join_node->node_children_head), 3);

			list_for_each(pos2, join_node->node_children_head)
			{
				if (j == 0) {
					table_node = list_entry(pos2, typeof(*table_node), head);
					CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
					CU_ASSERT_STRING_EQUAL(table_node->table_name, "A");
					CU_ASSERT_FALSE(list_is_empty(&table_node->head));
					CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);
				} else if (j == 1) {
					table_node = list_entry(pos2, typeof(*table_node), head);
					CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
					CU_ASSERT_STRING_EQUAL(table_node->table_name, "B");
					CU_ASSERT_FALSE(list_is_empty(&table_node->head));
					CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);
				} else {
					onexpr_node = list_entry(pos2, typeof(*onexpr_node), head);
					CU_ASSERT_EQUAL(onexpr_node->node_type, AST_TYPE_SEL_ONEXPR);
					CU_ASSERT_FALSE(list_is_empty(&onexpr_node->head));
					CU_ASSERT_EQUAL(list_length(onexpr_node->node_children_head), 1);

					list_for_each(pos3, onexpr_node->node_children_head)
					{
						cmp_node = list_entry(pos3, typeof(*cmp_node), head);
						CU_ASSERT_EQUAL(cmp_node->node_type, AST_TYPE_SEL_CMP);
						CU_ASSERT_EQUAL(cmp_node->cmp_type, AST_CMP_EQUALS_OP);
						CU_ASSERT_FALSE(list_is_empty(&cmp_node->head));
						CU_ASSERT_EQUAL(list_length(cmp_node->node_children_head), 2);

						list_for_each(pos4, cmp_node->node_children_head)
						{
							if (k == 0) {
								field_node = list_entry(pos4, typeof(*field_node),
											head);
								CU_ASSERT_EQUAL(field_node->node_type,
										AST_TYPE_SEL_FIELDNAME);
								CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
								CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
								CU_ASSERT_FALSE(list_is_empty(&field_node->head));
								CU_ASSERT_EQUAL(list_length(
										field_node->node_children_head),
										0);
							} else {
								field_node = list_entry(pos4, typeof(*field_node),
											head);
								CU_ASSERT_EQUAL(field_node->node_type,
										AST_TYPE_SEL_FIELDNAME);
								CU_ASSERT_STRING_EQUAL(field_node->col_name, "f2");
								CU_ASSERT_STRING_EQUAL(field_node->table_name, "B");
								CU_ASSERT_FALSE(list_is_empty(&field_node->head));
								CU_ASSERT_EQUAL(list_length(
										field_node->node_children_head),
										0);
							}
							k++;
						}
					}
				}
				j++;

			}

		}
		i++;

	}

	queue_free(&ct);
	ast_free(root);
}

// @formatter:off
static void select_case_14(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_sel_select_node *select_node;
	struct ast_sel_fieldname_node *field_node;
	struct ast_sel_table_node *table_node;
	struct ast_sel_join_node *join_node_1, *join_node_2;
	struct ast_sel_onexpr_node *onexpr_node;
	struct ast_sel_cmp_node *cmp_node;
	struct list_head *pos1, *pos2, *pos3, *pos4, *pos5;
	int i = 0, j = 0, k = 0, l = 0, m = 0;

	parse_stmt("SELECT A.f1, B.f2 FROM A JOIN B ON A.f1 = B.f2 JOIN C ON B.f2 = C.f3;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	select_node = (typeof(select_node))root;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT(list_is_empty(&select_node->head));
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 3);

	list_for_each(pos1, select_node->node_children_head)
	{
		if (i == 0) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
			CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
		} else if (i == 1) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "f2");
			CU_ASSERT_STRING_EQUAL(field_node->table_name, "B");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
		} else {
			join_node_1 = list_entry(pos1, typeof(*join_node_1), head);
			CU_ASSERT_EQUAL(join_node_1->node_type, AST_TYPE_SEL_JOIN);
			CU_ASSERT_EQUAL(join_node_1->join_type, AST_SEL_JOIN_INNER);
			CU_ASSERT_FALSE(list_is_empty(&join_node_1->head));
			CU_ASSERT_EQUAL(list_length(join_node_1->node_children_head), 3);

			list_for_each(pos2, join_node_1->node_children_head)
			{
				if (j == 0) {
					join_node_2 = list_entry(pos2, typeof(*join_node_2), head);
					CU_ASSERT_EQUAL(join_node_2->node_type, AST_TYPE_SEL_JOIN);
					CU_ASSERT_EQUAL(join_node_2->join_type, AST_SEL_JOIN_INNER);
					CU_ASSERT_FALSE(list_is_empty(&join_node_2->head));
					CU_ASSERT_EQUAL(list_length(join_node_2->node_children_head), 3);

					list_for_each(pos3, join_node_2->node_children_head)
					{
						if (k == 0){
							table_node = list_entry(pos3, typeof(*table_node), head);
							CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
							CU_ASSERT_STRING_EQUAL(table_node->table_name, "A");
							CU_ASSERT_FALSE(list_is_empty(&table_node->head));
							CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);
						} else if (k == 1){
							table_node = list_entry(pos3, typeof(*table_node), head);
							CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
							CU_ASSERT_STRING_EQUAL(table_node->table_name, "B");
							CU_ASSERT_FALSE(list_is_empty(&table_node->head));
							CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);
						} else {
							onexpr_node = list_entry(pos3, typeof(*onexpr_node), head);
							CU_ASSERT_EQUAL(onexpr_node->node_type, AST_TYPE_SEL_ONEXPR);
							CU_ASSERT_FALSE(list_is_empty(&onexpr_node->head));
							CU_ASSERT_EQUAL(list_length(onexpr_node->node_children_head), 1);

							list_for_each(pos4, onexpr_node->node_children_head)
							{
								cmp_node = list_entry(pos4, typeof(*cmp_node), head);
								CU_ASSERT_EQUAL(cmp_node->node_type, AST_TYPE_SEL_CMP);
								CU_ASSERT_EQUAL(cmp_node->cmp_type, AST_CMP_EQUALS_OP);
								CU_ASSERT_FALSE(list_is_empty(&cmp_node->head));
								CU_ASSERT_EQUAL(list_length(cmp_node->node_children_head), 2);

								list_for_each(pos5, cmp_node->node_children_head)
								{
									if (l == 0) {
										field_node = list_entry(pos5, typeof(*field_node), head);
										CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
										CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
										CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
										CU_ASSERT_FALSE(list_is_empty(&field_node->head));
										CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
									} else {
										field_node = list_entry(pos5, typeof(*field_node), head);
										CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
										CU_ASSERT_STRING_EQUAL(field_node->col_name, "f2");
										CU_ASSERT_STRING_EQUAL(field_node->table_name, "B");
										CU_ASSERT_FALSE(list_is_empty(&field_node->head));
										CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
									}
									l++;
								}
							}
						}
						k++;
					}

				} else if (j == 1) {
					table_node = list_entry(pos2, typeof(*table_node), head);
					CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
					CU_ASSERT_STRING_EQUAL(table_node->table_name, "C");
					CU_ASSERT_FALSE(list_is_empty(&table_node->head));
					CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);
				} else {
					onexpr_node = list_entry(pos2, typeof(*onexpr_node), head);
					CU_ASSERT_EQUAL(onexpr_node->node_type, AST_TYPE_SEL_ONEXPR);
					CU_ASSERT_FALSE(list_is_empty(&onexpr_node->head));
					CU_ASSERT_EQUAL(list_length(onexpr_node->node_children_head), 1);

					list_for_each(pos3, onexpr_node->node_children_head)
					{
						cmp_node = list_entry(pos3, typeof(*cmp_node), head);
						CU_ASSERT_EQUAL(cmp_node->node_type, AST_TYPE_SEL_CMP);
						CU_ASSERT_EQUAL(cmp_node->cmp_type, AST_CMP_EQUALS_OP);
						CU_ASSERT_FALSE(list_is_empty(&cmp_node->head));
						CU_ASSERT_EQUAL(list_length(cmp_node->node_children_head), 2);

						list_for_each(pos4, cmp_node->node_children_head)
						{
							if (m == 0) {
								field_node = list_entry(pos4, typeof(*field_node), head);
								CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
								CU_ASSERT_STRING_EQUAL(field_node->col_name, "f2");
								CU_ASSERT_STRING_EQUAL(field_node->table_name, "B");
								CU_ASSERT_FALSE(list_is_empty(&field_node->head));
								CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
							} else {
								field_node = list_entry(pos4, typeof(*field_node), head);
								CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
								CU_ASSERT_STRING_EQUAL(field_node->col_name, "f3");
								CU_ASSERT_STRING_EQUAL(field_node->table_name, "C");
								CU_ASSERT_FALSE(list_is_empty(&field_node->head));
								CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
							}
							m++;
						}
					}
				}
				j++;

			}

		}
		i++;

	}

	queue_free(&ct);
	ast_free(root);
}
// @formatter:on

static void select_case_15(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_sel_select_node *select_node;
	struct ast_sel_fieldname_node *field_node;
	struct ast_sel_table_node *table_node;
	struct ast_sel_groupby_node *groupby_node;
	struct list_head *pos1, *pos2;
	int i = 0, j = 0;

	parse_stmt("SELECT A.f1, A.f2 FROM A GROUP BY A.f1, A.f2;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	select_node = (typeof(select_node))root;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT(list_is_empty(&select_node->head));
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 4);

	list_for_each(pos1, select_node->node_children_head)
	{
		if (i == 0) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
			CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
		} else if (i == 1) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "f2");
			CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
		} else if (i == 2) {
			table_node = list_entry(pos1, typeof(*table_node), head);
			CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
			CU_ASSERT_STRING_EQUAL(table_node->table_name, "A");
			CU_ASSERT_FALSE(list_is_empty(&table_node->head));
			CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);
		} else {
			groupby_node = list_entry(pos1, typeof(*groupby_node), head);
			CU_ASSERT_EQUAL(groupby_node->node_type, AST_TYPE_SEL_GROUPBY);
			CU_ASSERT_FALSE(list_is_empty(&groupby_node->head));
			CU_ASSERT_EQUAL(list_length(groupby_node->node_children_head), 2);

			list_for_each(pos2, groupby_node->node_children_head)
			{

				if (j == 0) {
					field_node = list_entry(pos2, typeof(*field_node), head);
					CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
					CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
					CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
					CU_ASSERT_FALSE(list_is_empty(&field_node->head));
					CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
				} else {
					field_node = list_entry(pos2, typeof(*field_node), head);
					CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
					CU_ASSERT_STRING_EQUAL(field_node->col_name, "f2");
					CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
					CU_ASSERT_FALSE(list_is_empty(&field_node->head));
					CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
				}
				j++;
			}
		}
		i++;

	}

	queue_free(&ct);
	ast_free(root);
}

static void select_case_16(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_sel_select_node *select_node;
	struct ast_sel_fieldname_node *field_node;
	struct ast_sel_table_node *table_node;
	struct ast_sel_orderbylist_node *orderbylist_node;
	struct ast_sel_orderbyitem_node *orderbyitem_node;
	struct list_head *pos1, *pos2, *pos3;
	int i = 0;

	parse_stmt("SELECT A.name FROM A ORDER BY A.name;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	select_node = (typeof(select_node))root;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT(list_is_empty(&select_node->head));
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 3);

	list_for_each(pos1, select_node->node_children_head)
	{
		if (i == 0) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "name");
			CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
		} else if (i == 1) {
			table_node = list_entry(pos1, typeof(*table_node), head);
			CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
			CU_ASSERT_STRING_EQUAL(table_node->table_name, "A");
			CU_ASSERT_FALSE(list_is_empty(&table_node->head));
			CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);
		} else {
			orderbylist_node = list_entry(pos1, typeof(*orderbylist_node), head);
			CU_ASSERT_EQUAL(orderbylist_node->node_type, AST_TYPE_SEL_ORDERBYLIST);
			CU_ASSERT_FALSE(list_is_empty(&orderbylist_node->head));
			CU_ASSERT_EQUAL(list_length(orderbylist_node->node_children_head), 1);

			list_for_each(pos2, orderbylist_node->node_children_head)
			{
				orderbyitem_node = list_entry(pos2, typeof(*orderbyitem_node), head);
				CU_ASSERT_EQUAL(orderbyitem_node->node_type, AST_TYPE_SEL_ORDERBYITEM);
				CU_ASSERT_EQUAL(orderbyitem_node->direction, AST_SEL_ORDERBY_ASC);
				CU_ASSERT_FALSE(list_is_empty(&orderbyitem_node->head));
				CU_ASSERT_EQUAL(list_length(orderbyitem_node->node_children_head), 1);

				list_for_each(pos3, orderbyitem_node->node_children_head)
				{
					field_node = list_entry(pos3, typeof(*field_node), head);
					CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
					CU_ASSERT_STRING_EQUAL(field_node->col_name, "name");
					CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
					CU_ASSERT_FALSE(list_is_empty(&field_node->head));
					CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
				}
			}
		}
		i++;

	}

	queue_free(&ct);
	ast_free(root);
}

static void select_case_17(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_sel_select_node *select_node;
	struct ast_sel_fieldname_node *field_node;
	struct ast_sel_table_node *table_node;
	struct ast_sel_orderbylist_node *orderbylist_node;
	struct ast_sel_orderbyitem_node *orderbyitem_node;
	struct list_head *pos1, *pos2, *pos3;
	int i = 0;

	parse_stmt("SELECT A.name FROM A ORDER BY A.name DESC;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	select_node = (typeof(select_node))root;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT(list_is_empty(&select_node->head));
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 3);

	list_for_each(pos1, select_node->node_children_head)
	{
		if (i == 0) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "name");
			CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
		} else if (i == 1) {
			table_node = list_entry(pos1, typeof(*table_node), head);
			CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
			CU_ASSERT_STRING_EQUAL(table_node->table_name, "A");
			CU_ASSERT_FALSE(list_is_empty(&table_node->head));
			CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);
		} else {
			orderbylist_node = list_entry(pos1, typeof(*orderbylist_node), head);
			CU_ASSERT_EQUAL(orderbylist_node->node_type, AST_TYPE_SEL_ORDERBYLIST);
			CU_ASSERT_FALSE(list_is_empty(&orderbylist_node->head));
			CU_ASSERT_EQUAL(list_length(orderbylist_node->node_children_head), 1);

			list_for_each(pos2, orderbylist_node->node_children_head)
			{
				orderbyitem_node = list_entry(pos2, typeof(*orderbyitem_node), head);
				CU_ASSERT_EQUAL(orderbyitem_node->node_type, AST_TYPE_SEL_ORDERBYITEM);
				CU_ASSERT_EQUAL(orderbyitem_node->direction, AST_SEL_ORDERBY_DESC);
				CU_ASSERT_FALSE(list_is_empty(&orderbyitem_node->head));
				CU_ASSERT_EQUAL(list_length(orderbyitem_node->node_children_head), 1);

				list_for_each(pos3, orderbyitem_node->node_children_head)
				{
					field_node = list_entry(pos3, typeof(*field_node), head);
					CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
					CU_ASSERT_STRING_EQUAL(field_node->col_name, "name");
					CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
					CU_ASSERT_FALSE(list_is_empty(&field_node->head));
					CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
				}
			}
		}
		i++;

	}

	queue_free(&ct);
	ast_free(root);
}

static void select_case_18(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_sel_select_node *select_node;
	struct ast_sel_fieldname_node *field_node;
	struct ast_sel_table_node *table_node;
	struct ast_sel_groupby_node *groupby_node;
	struct ast_sel_having_node *having_node;
	struct ast_sel_cmp_node *cmp_node;
	struct ast_sel_exprval_node *val_node;
	struct ast_sel_count_node *count_node;
	struct list_head *pos1, *pos2, *pos3, *pos4;
	int i = 0, j = 0;

	parse_stmt("SELECT A.f2 FROM A GROUP BY A.f1 HAVING COUNT(A.f1) > 5;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	select_node = (typeof(select_node))root;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT(list_is_empty(&select_node->head));
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 4);

	list_for_each(pos1, select_node->node_children_head)
	{
		if (i == 0) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "f2");
			CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
		} else if (i == 1) {
			table_node = list_entry(pos1, typeof(*table_node), head);
			CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
			CU_ASSERT_STRING_EQUAL(table_node->table_name, "A");
			CU_ASSERT_FALSE(list_is_empty(&table_node->head));
			CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);
		} else if (i == 2) {
			groupby_node = list_entry(pos1, typeof(*groupby_node), head);
			CU_ASSERT_EQUAL(groupby_node->node_type, AST_TYPE_SEL_GROUPBY);
			CU_ASSERT_FALSE(list_is_empty(&groupby_node->head));
			CU_ASSERT_EQUAL(list_length(groupby_node->node_children_head), 1);

			list_for_each(pos2, groupby_node->node_children_head)
			{

				field_node = list_entry(pos2, typeof(*field_node), head);
				CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
				CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
				CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
				CU_ASSERT_FALSE(list_is_empty(&field_node->head));
				CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
			}
		} else {
			having_node = list_entry(pos1, typeof(*having_node), head);
			CU_ASSERT_EQUAL(having_node->node_type, AST_TYPE_SEL_HAVING);
			CU_ASSERT_FALSE(list_is_empty(&having_node->head));
			CU_ASSERT_EQUAL(list_length(having_node->node_children_head), 1);

			list_for_each(pos2, having_node->node_children_head)
			{
				cmp_node = list_entry(pos2, typeof(*cmp_node), head);
				CU_ASSERT_EQUAL(cmp_node->node_type, AST_TYPE_SEL_CMP);
				CU_ASSERT_EQUAL(cmp_node->cmp_type, AST_CMP_GT_OP);
				CU_ASSERT_FALSE(list_is_empty(&cmp_node->head));
				CU_ASSERT_EQUAL(list_length(cmp_node->node_children_head), 2);

				list_for_each(pos3, cmp_node->node_children_head)
				{
					if (j == 0) {
						count_node = list_entry(pos3, typeof(*count_node), head);
						CU_ASSERT_EQUAL(count_node->node_type, AST_TYPE_SEL_COUNT);
						CU_ASSERT_FALSE(count_node->all);
						CU_ASSERT_FALSE(list_is_empty(&count_node->head));
						CU_ASSERT_EQUAL(list_length(count_node->node_children_head), 1)

						list_for_each(pos4, count_node->node_children_head)
						{
							field_node = list_entry(pos4, typeof(*field_node), head);
							CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
							CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
							CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
							CU_ASSERT_FALSE(list_is_empty(&field_node->head));
							CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
						}

					} else {
						val_node = list_entry(pos3, typeof(*val_node), head);
						CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_SEL_EXPRVAL);
						CU_ASSERT(val_node->value_type.is_intnum);
						CU_ASSERT_EQUAL(val_node->int_val, 5);
						CU_ASSERT_FALSE(list_is_empty(&val_node->head));
						CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);
					}
					j++;
				}
			}
		}
		i++;

	}

	queue_free(&ct);
	ast_free(root);
}

static void select_case_19(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_sel_select_node *select_node;
	struct ast_sel_fieldname_node *field_node;
	struct ast_sel_table_node *table_node;
	struct ast_sel_where_node *where_node;
	struct ast_sel_like_node *like_node;
	struct ast_sel_exprval_node *val_node;
	struct list_head *pos1, *pos2, *pos3;
	int i = 0, j = 0;

	parse_stmt("SELECT A.f1 FROM A WHERE A.f1 like 'MidoriDB%';", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	select_node = (typeof(select_node))root;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT(list_is_empty(&select_node->head));
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 3);

	list_for_each(pos1, select_node->node_children_head)
	{
		if (i == 0) {
			field_node = list_entry(pos1, typeof(*field_node), head);
			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
			CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);
		} else if (i == 1) {
			table_node = list_entry(pos1, typeof(*table_node), head);
			CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
			CU_ASSERT_STRING_EQUAL(table_node->table_name, "A");
			CU_ASSERT_FALSE(list_is_empty(&table_node->head));
			CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);
		} else {
			where_node = list_entry(pos1, typeof(*where_node), head);
			CU_ASSERT_EQUAL(where_node->node_type, AST_TYPE_SEL_WHERE);
			CU_ASSERT_FALSE(list_is_empty(&where_node->head));
			CU_ASSERT_EQUAL(list_length(where_node->node_children_head), 1);

			list_for_each(pos2, where_node->node_children_head)
			{
				like_node = list_entry(pos2, typeof(*like_node), head);
				CU_ASSERT_EQUAL(like_node->node_type, AST_TYPE_SEL_LIKE);
				CU_ASSERT_FALSE(like_node->negate);
				CU_ASSERT_FALSE(list_is_empty(&like_node->head));
				CU_ASSERT_EQUAL(list_length(like_node->node_children_head), 2);

				list_for_each(pos3, like_node->node_children_head)
				{
					if (j == 0) {
						field_node = list_entry(pos3, typeof(*field_node), head);
						CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
						CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
						CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
						CU_ASSERT_FALSE(list_is_empty(&field_node->head));
						CU_ASSERT(list_is_empty(field_node->node_children_head));
					} else {
						val_node = list_entry(pos3, typeof(*val_node), head);
						CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_SEL_EXPRVAL);
						CU_ASSERT(val_node->value_type.is_str);
						CU_ASSERT_STRING_EQUAL(val_node->str_val, "MidoriDB%");
						CU_ASSERT_FALSE(list_is_empty(&val_node->head));
						CU_ASSERT(list_is_empty(val_node->node_children_head));
					}
					j++;
				}
			}
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
	/* DISTINCT + ALL + single table + no where-clause */
	select_case_5();
	/* multiple fields + single table + no where-clause */
	select_case_6();
	/* multiple fields + single table + no where-clause + limit clause */
	select_case_7();
	/* multiple fields + alias + single table + no where-clause + limit clause */
	select_case_8();
	/* multiple fields + multiple tables + no where-clause */
	select_case_9();
	/* multiple fields + multiple tables + alias + no where-clause */
	select_case_10();
	/* multiple fields + multiple tables + no where-clause + qualified field names */
	select_case_11();
	/* multiple fields + multiple tables + with where-clause + qualified field names */
	select_case_12();
	/* multiple fields + multiple tables + single join + no where-clause + qualified field names */
	select_case_13();
	/* multiple fields + multiple tables + multiple joins + no where-clause + qualified field names */
	select_case_14();
	/* multiple fields + single table + no where-clause + group by */
	select_case_15();
	/* single field + single table + no where-clause + order by - default */
	select_case_16();
	/* single field + single table + no where-clause + order by - explicit direction */
	select_case_17();
	/* single field + single table + no where-clause + group by + having  */
	select_case_18();
	/* single field + single table + where-clause + like  */
	select_case_19();
}
