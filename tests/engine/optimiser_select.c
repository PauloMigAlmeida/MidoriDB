/*
 * optimiser_select.c
 *
 *  Created on: 14/11/2023
 *      Author: paulo
 */

#include <tests/engine.h>
#include <engine/query.h>
#include <parser/ast.h>
#include <engine/optimiser.h>

extern struct ast_node* build_ast(char *stmt);

/**
 * Some statements require tables to exist before the semantic analysis take place.
 * So this a helper that should run before "helper" is invoked in some tests
 */
static void prep_helper(struct database *db, char *stmt)
{
	struct query_output *output;

	output = query_execute(db, stmt);
	CU_ASSERT_PTR_NOT_NULL_FATAL(output);
	CU_ASSERT_EQUAL_FATAL(output->status, ST_OK_EXECUTED)

	free(output);
}

static void select_case_1(void)
{
	struct database db = {0};
	struct query_output output = {0};
	struct ast_node *node;
	struct ast_sel_select_node *select_node;
	struct ast_sel_exprval_node *val_node;
	struct ast_sel_table_node *table_node;
	struct ast_sel_fieldname_node *field_node;
	struct list_head *pos;
	int i;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	prep_helper(&db, "CREATE TABLE A (f1 INT);");

	node = build_ast("SELECT f1 FROM A;");

	// before
	select_node = (typeof(select_node))node;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 2);
	CU_ASSERT(list_is_empty(&select_node->head));

	i = 0;
	list_for_each(pos, select_node->node_children_head)
	{
		if (i == 0) {
			val_node = list_entry(pos, typeof(*val_node), head);

			CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_SEL_EXPRVAL);
			CU_ASSERT_FALSE(list_is_empty(&val_node->head));
			CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);

			CU_ASSERT(val_node->value_type.is_name);
			CU_ASSERT_STRING_EQUAL(val_node->name_val, "f1");
		} else {
			table_node = list_entry(pos, typeof(*table_node), head);

			CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
			CU_ASSERT_FALSE(list_is_empty(&table_node->head));
			CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);

			CU_ASSERT_STRING_EQUAL(table_node->table_name, "A");
		}
		i++;
	}

	// optimise it
	CU_ASSERT_EQUAL(optimiser_run(&db, node, &output), MIDORIDB_OK);

	// after
	select_node = (typeof(select_node))node;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 2);
	CU_ASSERT(list_is_empty(&select_node->head));

	i = 0;
	list_for_each(pos, select_node->node_children_head)
	{
		if (i == 0) {
			field_node = list_entry(pos, typeof(*field_node), head);

			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);

			CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
		} else {
			table_node = list_entry(pos, typeof(*table_node), head);

			CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
			CU_ASSERT_FALSE(list_is_empty(&table_node->head));
			CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);

			CU_ASSERT_STRING_EQUAL(table_node->table_name, "A");
		}
		i++;
	}

	ast_free(node);
	database_close(&db);
}

void test_optimiser_select(void)
{
	/* replace simple exprval (name) with fqfield */
	select_case_1();
}
