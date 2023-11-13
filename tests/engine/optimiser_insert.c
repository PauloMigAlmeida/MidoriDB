/*
 * optimiser_insert.c
 *
 * Notes to myself:
 *
 * - tests skips (on purpose) semantic evaluation. This
 * 	allows me to focus on the changes made to AST without
 * 	having to worry about running preparation commands such as CREATE
 * 	tables and so on.
 *
 *  Created on: 14/11/2023
 *      Author: paulo
 */


#include <tests/engine.h>
#include <engine/query.h>
#include <parser/ast.h>
#include <engine/optimiser.h>

extern struct ast_node* build_ast(char *stmt);

static void test_insert_1(void)
{
	struct database db = {0};
	struct query_output output = {0};
	struct ast_node *node;
	struct ast_ins_insvals_node *insert_node;
	struct list_head *pos1, *pos2;
	struct ast_ins_values_node *vals_entry;
	struct ast_ins_exprval_node *expval_entry;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	node = build_ast("INSERT INTO A VALUES ((2+2)*3);");
	CU_ASSERT_EQUAL(optimiser_run(&db, node, &output), MIDORIDB_OK);

	insert_node = (typeof(insert_node))node;
	CU_ASSERT_EQUAL(insert_node->node_type, AST_TYPE_INS_INSVALS);
	CU_ASSERT_EQUAL(insert_node->row_count, 1);
	CU_ASSERT_EQUAL(list_length(insert_node->node_children_head), 1);
	CU_ASSERT_STRING_EQUAL(insert_node->table_name, "A");
	CU_ASSERT(list_is_empty(&insert_node->head));
	CU_ASSERT_FALSE(insert_node->opt_column_list);

	list_for_each(pos1, insert_node->node_children_head)
	{
		vals_entry = list_entry(pos1, typeof(*vals_entry), head);
		CU_ASSERT_EQUAL(vals_entry->node_type, AST_TYPE_INS_VALUES);
		CU_ASSERT_FALSE(list_is_empty(&vals_entry->head));
		CU_ASSERT_EQUAL(list_length(vals_entry->node_children_head), 1);

		list_for_each(pos2, vals_entry->node_children_head)
		{
			expval_entry = list_entry(pos2, typeof(*expval_entry), head);

			CU_ASSERT_EQUAL(expval_entry->node_type, AST_TYPE_INS_EXPRVAL);
			CU_ASSERT_FALSE(list_is_empty(&expval_entry->head));
			CU_ASSERT_EQUAL(list_length(expval_entry->node_children_head), 0);

			CU_ASSERT(expval_entry->value_type.is_intnum);
			CU_ASSERT_EQUAL(expval_entry->int_val, 12);
		}

	}

	ast_free(node);
	database_close(&db);
}

static void test_insert_2(void)
{
	struct database db = {0};
	struct query_output output = {0};
	struct ast_node *node;
	struct ast_ins_insvals_node *insert_node;
	struct list_head *pos1, *pos2;
	struct ast_ins_values_node *vals_entry;
	struct ast_ins_exprval_node *expval_entry;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	node = build_ast("INSERT INTO A VALUES (6/(3*(9-(5/5))/4));");
	CU_ASSERT_EQUAL(optimiser_run(&db, node, &output), MIDORIDB_OK);

	insert_node = (typeof(insert_node))node;
	CU_ASSERT_EQUAL(insert_node->node_type, AST_TYPE_INS_INSVALS);
	CU_ASSERT_EQUAL(insert_node->row_count, 1);
	CU_ASSERT_EQUAL(list_length(insert_node->node_children_head), 1);
	CU_ASSERT_STRING_EQUAL(insert_node->table_name, "A");
	CU_ASSERT(list_is_empty(&insert_node->head));
	CU_ASSERT_FALSE(insert_node->opt_column_list);

	list_for_each(pos1, insert_node->node_children_head)
	{
		vals_entry = list_entry(pos1, typeof(*vals_entry), head);
		CU_ASSERT_EQUAL(vals_entry->node_type, AST_TYPE_INS_VALUES);
		CU_ASSERT_FALSE(list_is_empty(&vals_entry->head));
		CU_ASSERT_EQUAL(list_length(vals_entry->node_children_head), 1);

		list_for_each(pos2, vals_entry->node_children_head)
		{
			expval_entry = list_entry(pos2, typeof(*expval_entry), head);

			CU_ASSERT_EQUAL(expval_entry->node_type, AST_TYPE_INS_EXPRVAL);
			CU_ASSERT_FALSE(list_is_empty(&expval_entry->head));
			CU_ASSERT_EQUAL(list_length(expval_entry->node_children_head), 0);

			CU_ASSERT(expval_entry->value_type.is_intnum);
			CU_ASSERT_EQUAL(expval_entry->int_val, 1);
		}

	}

	ast_free(node);
	database_close(&db);
}

static void test_insert_3(void)
{
	struct database db = {0};
	struct query_output output = {0};
	struct ast_node *node;
	struct ast_ins_insvals_node *insert_node;
	struct list_head *pos1, *pos2;
	struct ast_ins_values_node *vals_entry;
	struct ast_ins_exprval_node *expval_entry;
	int i = 0;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	node = build_ast("INSERT INTO A VALUES (6/(3*(9-(5/5))/4), 6/2*(1+2));");
	CU_ASSERT_EQUAL(optimiser_run(&db, node, &output), MIDORIDB_OK);

	insert_node = (typeof(insert_node))node;
	CU_ASSERT_EQUAL(insert_node->node_type, AST_TYPE_INS_INSVALS);
	CU_ASSERT_EQUAL(insert_node->row_count, 1);
	CU_ASSERT_EQUAL(list_length(insert_node->node_children_head), 1);
	CU_ASSERT_STRING_EQUAL(insert_node->table_name, "A");
	CU_ASSERT(list_is_empty(&insert_node->head));
	CU_ASSERT_FALSE(insert_node->opt_column_list);

	list_for_each(pos1, insert_node->node_children_head)
	{
		vals_entry = list_entry(pos1, typeof(*vals_entry), head);
		CU_ASSERT_EQUAL(vals_entry->node_type, AST_TYPE_INS_VALUES);
		CU_ASSERT_FALSE(list_is_empty(&vals_entry->head));
		CU_ASSERT_EQUAL(list_length(vals_entry->node_children_head), 2);

		list_for_each(pos2, vals_entry->node_children_head)
		{
			expval_entry = list_entry(pos2, typeof(*expval_entry), head);

			CU_ASSERT_EQUAL(expval_entry->node_type, AST_TYPE_INS_EXPRVAL);
			CU_ASSERT_FALSE(list_is_empty(&expval_entry->head));
			CU_ASSERT_EQUAL(list_length(expval_entry->node_children_head), 0);

			if (i == 0) {
				CU_ASSERT(expval_entry->value_type.is_intnum);
				CU_ASSERT_EQUAL(expval_entry->int_val, 1);
			} else {
				CU_ASSERT(expval_entry->value_type.is_intnum);
				CU_ASSERT_EQUAL(expval_entry->int_val, 9);
			}
			i++;

		}

	}

	ast_free(node);
	database_close(&db);
}

static void test_insert_4(void)
{
	struct database db = {0};
	struct query_output output = {0};
	struct ast_node *node;
	struct ast_ins_insvals_node *insert_node;
	struct list_head *pos1, *pos2;
	struct ast_ins_values_node *vals_entry;
	struct ast_ins_exprval_node *expval_entry;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	node = build_ast("INSERT INTO A VALUES (6/(0*(9-(5/5))/4), 6/0*(1+2));");
	CU_ASSERT_EQUAL(optimiser_run(&db, node, &output), MIDORIDB_OK);

	insert_node = (typeof(insert_node))node;
	CU_ASSERT_EQUAL(insert_node->node_type, AST_TYPE_INS_INSVALS);
	CU_ASSERT_EQUAL(insert_node->row_count, 1);
	CU_ASSERT_EQUAL(list_length(insert_node->node_children_head), 1);
	CU_ASSERT_STRING_EQUAL(insert_node->table_name, "A");
	CU_ASSERT(list_is_empty(&insert_node->head));
	CU_ASSERT_FALSE(insert_node->opt_column_list);

	list_for_each(pos1, insert_node->node_children_head)
	{
		vals_entry = list_entry(pos1, typeof(*vals_entry), head);
		CU_ASSERT_EQUAL(vals_entry->node_type, AST_TYPE_INS_VALUES);
		CU_ASSERT_FALSE(list_is_empty(&vals_entry->head));
		CU_ASSERT_EQUAL(list_length(vals_entry->node_children_head), 2);

		list_for_each(pos2, vals_entry->node_children_head)
		{
			expval_entry = list_entry(pos2, typeof(*expval_entry), head);

			CU_ASSERT_EQUAL(expval_entry->node_type, AST_TYPE_INS_EXPRVAL);
			CU_ASSERT_FALSE(list_is_empty(&expval_entry->head));
			CU_ASSERT_EQUAL(list_length(expval_entry->node_children_head), 0);

			// both terms should have the same value (NULL)
			CU_ASSERT(expval_entry->value_type.is_null);
			CU_ASSERT_EQUAL(expval_entry->int_val, 0);
		}
	}

	ast_free(node);
	database_close(&db);
}

static void test_insert_5(void)
{
	struct database db = {0};
	struct query_output output = {0};
	struct ast_node *node;
	struct ast_ins_insvals_node *insert_node;
	struct list_head *pos1, *pos2;
	struct ast_ins_values_node *vals_entry;
	struct ast_ins_exprval_node *expval_entry;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	node = build_ast("INSERT INTO A VALUES ((2+2)%2);");
	CU_ASSERT_EQUAL(optimiser_run(&db, node, &output), MIDORIDB_OK);

	insert_node = (typeof(insert_node))node;
	CU_ASSERT_EQUAL(insert_node->node_type, AST_TYPE_INS_INSVALS);
	CU_ASSERT_EQUAL(insert_node->row_count, 1);
	CU_ASSERT_EQUAL(list_length(insert_node->node_children_head), 1);
	CU_ASSERT_STRING_EQUAL(insert_node->table_name, "A");
	CU_ASSERT(list_is_empty(&insert_node->head));
	CU_ASSERT_FALSE(insert_node->opt_column_list);

	list_for_each(pos1, insert_node->node_children_head)
	{
		vals_entry = list_entry(pos1, typeof(*vals_entry), head);
		CU_ASSERT_EQUAL(vals_entry->node_type, AST_TYPE_INS_VALUES);
		CU_ASSERT_FALSE(list_is_empty(&vals_entry->head));
		CU_ASSERT_EQUAL(list_length(vals_entry->node_children_head), 1);

		list_for_each(pos2, vals_entry->node_children_head)
		{
			expval_entry = list_entry(pos2, typeof(*expval_entry), head);

			CU_ASSERT_EQUAL(expval_entry->node_type, AST_TYPE_INS_EXPRVAL);
			CU_ASSERT_FALSE(list_is_empty(&expval_entry->head));
			CU_ASSERT_EQUAL(list_length(expval_entry->node_children_head), 0);

			CU_ASSERT(expval_entry->value_type.is_intnum);
			CU_ASSERT_EQUAL(expval_entry->int_val, 0);
		}

	}

	ast_free(node);
	database_close(&db);
}

static void test_insert_6(void)
{
	struct database db = {0};
	struct query_output output = {0};
	struct ast_node *node;
	struct ast_ins_insvals_node *insert_node;
	struct list_head *pos1, *pos2;
	struct ast_ins_values_node *vals_entry;
	struct ast_ins_exprval_node *expval_entry;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	node = build_ast("INSERT INTO A VALUES (-(2+2)*2);");

	CU_ASSERT_EQUAL(optimiser_run(&db, node, &output), MIDORIDB_OK);

	insert_node = (typeof(insert_node))node;
	CU_ASSERT_EQUAL(insert_node->node_type, AST_TYPE_INS_INSVALS);
	CU_ASSERT_EQUAL(insert_node->row_count, 1);
	CU_ASSERT_EQUAL(list_length(insert_node->node_children_head), 1);
	CU_ASSERT_STRING_EQUAL(insert_node->table_name, "A");
	CU_ASSERT(list_is_empty(&insert_node->head));
	CU_ASSERT_FALSE(insert_node->opt_column_list);

	list_for_each(pos1, insert_node->node_children_head)
	{
		vals_entry = list_entry(pos1, typeof(*vals_entry), head);
		CU_ASSERT_EQUAL(vals_entry->node_type, AST_TYPE_INS_VALUES);
		CU_ASSERT_FALSE(list_is_empty(&vals_entry->head));
		CU_ASSERT_EQUAL(list_length(vals_entry->node_children_head), 1);

		list_for_each(pos2, vals_entry->node_children_head)
		{
			expval_entry = list_entry(pos2, typeof(*expval_entry), head);

			CU_ASSERT_EQUAL(expval_entry->node_type, AST_TYPE_INS_EXPRVAL);
			CU_ASSERT_FALSE(list_is_empty(&expval_entry->head));
			CU_ASSERT_EQUAL(list_length(expval_entry->node_children_head), 0);

			CU_ASSERT(expval_entry->value_type.is_intnum);
			CU_ASSERT_EQUAL(expval_entry->int_val, -8);
		}

	}

	ast_free(node);
	database_close(&db);
}

static void test_insert_7(void)
{
	struct database db = {0};
	struct query_output output = {0};
	struct ast_node *node;
	struct ast_ins_insvals_node *insert_node;
	struct list_head *pos1, *pos2;
	struct ast_ins_values_node *vals_entry;
	struct ast_ins_exprval_node *expval_entry;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	node = build_ast("INSERT INTO A VALUES ((2.0+2.0)*3.0);");
	CU_ASSERT_EQUAL(optimiser_run(&db, node, &output), MIDORIDB_OK);

	insert_node = (typeof(insert_node))node;
	CU_ASSERT_EQUAL(insert_node->node_type, AST_TYPE_INS_INSVALS);
	CU_ASSERT_EQUAL(insert_node->row_count, 1);
	CU_ASSERT_EQUAL(list_length(insert_node->node_children_head), 1);
	CU_ASSERT_STRING_EQUAL(insert_node->table_name, "A");
	CU_ASSERT(list_is_empty(&insert_node->head));
	CU_ASSERT_FALSE(insert_node->opt_column_list);

	list_for_each(pos1, insert_node->node_children_head)
	{
		vals_entry = list_entry(pos1, typeof(*vals_entry), head);
		CU_ASSERT_EQUAL(vals_entry->node_type, AST_TYPE_INS_VALUES);
		CU_ASSERT_FALSE(list_is_empty(&vals_entry->head));
		CU_ASSERT_EQUAL(list_length(vals_entry->node_children_head), 1);

		list_for_each(pos2, vals_entry->node_children_head)
		{
			expval_entry = list_entry(pos2, typeof(*expval_entry), head);

			CU_ASSERT_EQUAL(expval_entry->node_type, AST_TYPE_INS_EXPRVAL);
			CU_ASSERT_FALSE(list_is_empty(&expval_entry->head));
			CU_ASSERT_EQUAL(list_length(expval_entry->node_children_head), 0);

			CU_ASSERT(expval_entry->value_type.is_approxnum);
			CU_ASSERT_EQUAL(expval_entry->double_val, 12.0);
		}

	}

	ast_free(node);
	database_close(&db);
}

static void test_insert_8(void)
{
	struct database db = {0};
	struct query_output output = {0};
	struct ast_node *node;
	struct ast_ins_insvals_node *insert_node;
	struct list_head *pos1, *pos2;
	struct ast_ins_values_node *vals_entry;
	struct ast_ins_exprval_node *expval_entry;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	node = build_ast("INSERT INTO A VALUES (6.0/(3.0*(9.0-(5.0/5.0))/4.0));");
	CU_ASSERT_EQUAL(optimiser_run(&db, node, &output), MIDORIDB_OK);

	insert_node = (typeof(insert_node))node;
	CU_ASSERT_EQUAL(insert_node->node_type, AST_TYPE_INS_INSVALS);
	CU_ASSERT_EQUAL(insert_node->row_count, 1);
	CU_ASSERT_EQUAL(list_length(insert_node->node_children_head), 1);
	CU_ASSERT_STRING_EQUAL(insert_node->table_name, "A");
	CU_ASSERT(list_is_empty(&insert_node->head));
	CU_ASSERT_FALSE(insert_node->opt_column_list);

	list_for_each(pos1, insert_node->node_children_head)
	{
		vals_entry = list_entry(pos1, typeof(*vals_entry), head);
		CU_ASSERT_EQUAL(vals_entry->node_type, AST_TYPE_INS_VALUES);
		CU_ASSERT_FALSE(list_is_empty(&vals_entry->head));
		CU_ASSERT_EQUAL(list_length(vals_entry->node_children_head), 1);

		list_for_each(pos2, vals_entry->node_children_head)
		{
			expval_entry = list_entry(pos2, typeof(*expval_entry), head);

			CU_ASSERT_EQUAL(expval_entry->node_type, AST_TYPE_INS_EXPRVAL);
			CU_ASSERT_FALSE(list_is_empty(&expval_entry->head));
			CU_ASSERT_EQUAL(list_length(expval_entry->node_children_head), 0);

			CU_ASSERT(expval_entry->value_type.is_approxnum);
			CU_ASSERT_EQUAL(expval_entry->double_val, 1.0);
		}

	}

	ast_free(node);
	database_close(&db);
}

static void test_insert_9(void)
{
	struct database db = {0};
	struct query_output output = {0};
	struct ast_node *node;
	struct ast_ins_insvals_node *insert_node;
	struct list_head *pos1, *pos2;
	struct ast_ins_values_node *vals_entry;
	struct ast_ins_exprval_node *expval_entry;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	node = build_ast("INSERT INTO A VALUES (6.0/(0.0*(9.0-(5.0/5.0))/4.0), 6.0/0.0*(1.0+2.0));");
	CU_ASSERT_EQUAL(optimiser_run(&db, node, &output), MIDORIDB_OK);

	insert_node = (typeof(insert_node))node;
	CU_ASSERT_EQUAL(insert_node->node_type, AST_TYPE_INS_INSVALS);
	CU_ASSERT_EQUAL(insert_node->row_count, 1);
	CU_ASSERT_EQUAL(list_length(insert_node->node_children_head), 1);
	CU_ASSERT_STRING_EQUAL(insert_node->table_name, "A");
	CU_ASSERT(list_is_empty(&insert_node->head));
	CU_ASSERT_FALSE(insert_node->opt_column_list);

	list_for_each(pos1, insert_node->node_children_head)
	{
		vals_entry = list_entry(pos1, typeof(*vals_entry), head);
		CU_ASSERT_EQUAL(vals_entry->node_type, AST_TYPE_INS_VALUES);
		CU_ASSERT_FALSE(list_is_empty(&vals_entry->head));
		CU_ASSERT_EQUAL(list_length(vals_entry->node_children_head), 2);

		list_for_each(pos2, vals_entry->node_children_head)
		{
			expval_entry = list_entry(pos2, typeof(*expval_entry), head);

			CU_ASSERT_EQUAL(expval_entry->node_type, AST_TYPE_INS_EXPRVAL);
			CU_ASSERT_FALSE(list_is_empty(&expval_entry->head));
			CU_ASSERT_EQUAL(list_length(expval_entry->node_children_head), 0);

			// both terms should have the same value (NULL)
			CU_ASSERT(expval_entry->value_type.is_null);
			CU_ASSERT_EQUAL(expval_entry->double_val, 0);
		}
	}

	ast_free(node);
	database_close(&db);
}

static void test_insert_10(void)
{
	struct database db = {0};
	struct query_output output = {0};
	struct ast_node *node;
	struct ast_ins_insvals_node *insert_node;
	struct list_head *pos1, *pos2;
	struct ast_ins_values_node *vals_entry;
	struct ast_ins_exprval_node *expval_entry;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	node = build_ast("INSERT INTO A VALUES (6.0 / (3.0 * (9.0 -(-(5.0 / 5.0))) / 4.0));");
	CU_ASSERT_EQUAL(optimiser_run(&db, node, &output), MIDORIDB_OK);

	insert_node = (typeof(insert_node))node;
	CU_ASSERT_EQUAL(insert_node->node_type, AST_TYPE_INS_INSVALS);
	CU_ASSERT_EQUAL(insert_node->row_count, 1);
	CU_ASSERT_EQUAL(list_length(insert_node->node_children_head), 1);
	CU_ASSERT_STRING_EQUAL(insert_node->table_name, "A");
	CU_ASSERT(list_is_empty(&insert_node->head));
	CU_ASSERT_FALSE(insert_node->opt_column_list);

	list_for_each(pos1, insert_node->node_children_head)
	{
		vals_entry = list_entry(pos1, typeof(*vals_entry), head);
		CU_ASSERT_EQUAL(vals_entry->node_type, AST_TYPE_INS_VALUES);
		CU_ASSERT_FALSE(list_is_empty(&vals_entry->head));
		CU_ASSERT_EQUAL(list_length(vals_entry->node_children_head), 1);

		list_for_each(pos2, vals_entry->node_children_head)
		{
			expval_entry = list_entry(pos2, typeof(*expval_entry), head);

			CU_ASSERT_EQUAL(expval_entry->node_type, AST_TYPE_INS_EXPRVAL);
			CU_ASSERT_FALSE(list_is_empty(&expval_entry->head));
			CU_ASSERT_EQUAL(list_length(expval_entry->node_children_head), 0);

			CU_ASSERT(expval_entry->value_type.is_approxnum);
			CU_ASSERT_EQUAL(expval_entry->double_val, 0.8);
		}

	}

	ast_free(node);
	database_close(&db);
}

void test_optimiser_insert(void)
{
	/* single expr; int value; single recurssion; no div-by-0 */
	test_insert_1();
	/* single expr; int value; multi recurssion; no div-by-0 */
	test_insert_2();
	/* multi expr; int value; multi recurssion; no div-by-0 */
	test_insert_3();
	/* multi expr; int value; multi recurssion; div-by-0 */
	test_insert_4();
	/* single expr; int value; mod operator; no div-by-0 */
	test_insert_5();
	/* single expr; int value; negation; no div-by-0 */
	test_insert_6();

	/* single expr; double value; multi recurssion; no div-by-0 */
	test_insert_7();
	/* multi expr; double value; multi recurssion; no div-by-0 */
	test_insert_8();
	/* multi expr; double value; multi recurssion; div-by-0 */
	test_insert_9();
	/* single expr; double value; multi recurssion; negation; no div-by-0 */
	test_insert_10();
}
