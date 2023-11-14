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

static void select_case_2(void)
{
	struct database db = {0};
	struct query_output output = {0};
	struct ast_node *node;
	struct ast_sel_select_node *select_node;
	struct ast_sel_alias_node *alias_node;
	struct ast_sel_exprval_node *val_node;
	struct ast_sel_table_node *table_node;
	struct ast_sel_fieldname_node *field_node;
	struct list_head *pos1, *pos2;
	int i;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	prep_helper(&db, "CREATE TABLE A (f1 INT);");

	node = build_ast("SELECT f1 as val FROM A;");

	// before
	select_node = (typeof(select_node))node;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 2);
	CU_ASSERT(list_is_empty(&select_node->head));

	i = 0;
	list_for_each(pos1, select_node->node_children_head)
	{
		if (i == 0) {
			alias_node = list_entry(pos1, typeof(*alias_node), head);
			CU_ASSERT_EQUAL(alias_node->node_type, AST_TYPE_SEL_ALIAS);
			CU_ASSERT_FALSE(list_is_empty(&alias_node->head));
			CU_ASSERT_EQUAL(list_length(alias_node->node_children_head), 1);
			CU_ASSERT_STRING_EQUAL(alias_node->alias_value, "val");

			list_for_each(pos2, alias_node->node_children_head)
			{
				val_node = list_entry(pos2, typeof(*val_node), head);

				CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_SEL_EXPRVAL);
				CU_ASSERT_FALSE(list_is_empty(&val_node->head));
				CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);

				CU_ASSERT(val_node->value_type.is_name);
				CU_ASSERT_STRING_EQUAL(val_node->name_val, "f1");
			}
		} else {
			table_node = list_entry(pos1, typeof(*table_node), head);

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
	list_for_each(pos1, select_node->node_children_head)
	{
		if (i == 0) {
			alias_node = list_entry(pos1, typeof(*alias_node), head);
			CU_ASSERT_EQUAL(alias_node->node_type, AST_TYPE_SEL_ALIAS);
			CU_ASSERT_FALSE(list_is_empty(&alias_node->head));
			CU_ASSERT_EQUAL(list_length(alias_node->node_children_head), 1);
			CU_ASSERT_STRING_EQUAL(alias_node->alias_value, "val");

			list_for_each(pos2, alias_node->node_children_head)
			{

				field_node = list_entry(pos2, typeof(*field_node), head);

				CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
				CU_ASSERT_FALSE(list_is_empty(&field_node->head));
				CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);

				CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
				CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
			}
		} else {
			table_node = list_entry(pos1, typeof(*table_node), head);

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

static void select_case_3(void)
{
	struct database db = {0};
	struct query_output output = {0};
	struct ast_node *node;
	struct ast_sel_select_node *select_node;
	struct ast_sel_table_node *table_node;
	struct ast_sel_fieldname_node *field_node;
	struct list_head *pos;
	int i;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	prep_helper(&db, "CREATE TABLE A (f1 INT);");

	node = build_ast("SELECT A.f1 FROM A;");

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

static void select_case_4(void)
{
	struct database db = {0};
	struct query_output output = {0};
	struct ast_node *node;
	struct ast_sel_select_node *select_node;
	struct ast_sel_table_node *table_node;
	struct ast_sel_fieldname_node *field_node;
	struct ast_sel_alias_node *alias_node;
	struct list_head *pos1, *pos2;
	int i;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	prep_helper(&db, "CREATE TABLE A (f1 INT);");

	node = build_ast("SELECT t.f1 FROM A t;");

	// before
	select_node = (typeof(select_node))node;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 2);
	CU_ASSERT(list_is_empty(&select_node->head));

	i = 0;
	list_for_each(pos1, select_node->node_children_head)
	{
		if (i == 0) {
			field_node = list_entry(pos1, typeof(*field_node), head);

			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);

			CU_ASSERT_STRING_EQUAL(field_node->table_name, "t");
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
		} else {

			alias_node = list_entry(pos1, typeof(*alias_node), head);
			CU_ASSERT_EQUAL(alias_node->node_type, AST_TYPE_SEL_ALIAS);
			CU_ASSERT_FALSE(list_is_empty(&alias_node->head));
			CU_ASSERT_EQUAL(list_length(alias_node->node_children_head), 1);
			CU_ASSERT_STRING_EQUAL(alias_node->alias_value, "t");

			list_for_each(pos2, alias_node->node_children_head)
			{

				table_node = list_entry(pos2, typeof(*table_node), head);

				CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
				CU_ASSERT_FALSE(list_is_empty(&table_node->head));
				CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);

				CU_ASSERT_STRING_EQUAL(table_node->table_name, "A");
			}
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
	list_for_each(pos1, select_node->node_children_head)
	{
		if (i == 0) {
			field_node = list_entry(pos1, typeof(*field_node), head);

			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);

			CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
		} else {

			table_node = list_entry(pos1, typeof(*table_node), head);

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

static void select_case_5(void)
{
	struct database db = {0};
	struct query_output output = {0};
	struct ast_node *node;
	struct ast_sel_select_node *select_node;
	struct ast_sel_selectall_node *selectall_node;
	struct ast_sel_table_node *table_node;
	struct ast_sel_fieldname_node *field_node;
	struct list_head *pos;
	int i;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	prep_helper(&db, "CREATE TABLE A (f1 INT, f2 INT);");

	node = build_ast("SELECT * FROM A;");

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
			selectall_node = list_entry(pos, typeof(*selectall_node), head);

			CU_ASSERT_EQUAL(selectall_node->node_type, AST_TYPE_SEL_SELECTALL);
			CU_ASSERT_FALSE(list_is_empty(&selectall_node->head));
			CU_ASSERT_EQUAL(list_length(selectall_node->node_children_head), 0);
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
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 3);
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
		} else if (i == 1) {
			field_node = list_entry(pos, typeof(*field_node), head);

			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);

			CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "f2");
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

static void select_case_6(void)
{
	struct database db = {0};
	struct query_output output = {0};
	struct ast_node *node;
	struct ast_sel_select_node *select_node;
	struct ast_sel_selectall_node *selectall_node;
	struct ast_sel_table_node *table_node;
	struct ast_sel_fieldname_node *field_node;
	struct ast_sel_join_node *join_node;
	struct ast_sel_onexpr_node *onexpr_node;
	struct ast_sel_alias_node *alias_node;
	struct ast_sel_cmp_node *cmp_node;
	struct list_head *pos1, *pos2, *pos3, *pos4;
	int i, j, k;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	prep_helper(&db, "CREATE TABLE A (f1 INT, f2 INT);");
	prep_helper(&db, "CREATE TABLE B (f1 INT, f2 INT);");

	node = build_ast("SELECT * FROM A t1 INNER JOIN B t2 ON t1.f1 = t2.f1;");

	// before
	select_node = (typeof(select_node))node;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 2);
	CU_ASSERT(list_is_empty(&select_node->head));

	i = 0;
	list_for_each(pos1, select_node->node_children_head)
	{
		if (i == 0) {
			selectall_node = list_entry(pos1, typeof(*selectall_node), head);

			CU_ASSERT_EQUAL(selectall_node->node_type, AST_TYPE_SEL_SELECTALL);
			CU_ASSERT_FALSE(list_is_empty(&selectall_node->head));
			CU_ASSERT_EQUAL(list_length(selectall_node->node_children_head), 0);
		} else {

			join_node = list_entry(pos1, typeof(*join_node), head);

			CU_ASSERT_EQUAL(join_node->node_type, AST_TYPE_SEL_JOIN);
			CU_ASSERT_FALSE(list_is_empty(&join_node->head));
			CU_ASSERT_EQUAL(list_length(join_node->node_children_head), 3);
			CU_ASSERT_EQUAL(join_node->join_type, AST_SEL_JOIN_INNER);

			j = 0;
			list_for_each(pos2, join_node->node_children_head)
			{
				if (j == 0) {
					alias_node = list_entry(pos2, typeof(*alias_node), head);
					CU_ASSERT_EQUAL(alias_node->node_type, AST_TYPE_SEL_ALIAS);
					CU_ASSERT_FALSE(list_is_empty(&alias_node->head));
					CU_ASSERT_EQUAL(list_length(alias_node->node_children_head), 1);
					CU_ASSERT_STRING_EQUAL(alias_node->alias_value, "t1");

					list_for_each(pos3, alias_node->node_children_head)
					{
						table_node = list_entry(pos3, typeof(*table_node), head);

						CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
						CU_ASSERT_FALSE(list_is_empty(&table_node->head));
						CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);

						CU_ASSERT_STRING_EQUAL(table_node->table_name, "A");
					}
				} else if (j == 1) {
					alias_node = list_entry(pos2, typeof(*alias_node), head);
					CU_ASSERT_EQUAL(alias_node->node_type, AST_TYPE_SEL_ALIAS);
					CU_ASSERT_FALSE(list_is_empty(&alias_node->head));
					CU_ASSERT_EQUAL(list_length(alias_node->node_children_head), 1);
					CU_ASSERT_STRING_EQUAL(alias_node->alias_value, "t2");

					list_for_each(pos3, alias_node->node_children_head)
					{
						table_node = list_entry(pos3, typeof(*table_node), head);

						CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
						CU_ASSERT_FALSE(list_is_empty(&table_node->head));
						CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);

						CU_ASSERT_STRING_EQUAL(table_node->table_name, "B");
					}

				} else {
					onexpr_node = list_entry(pos2, typeof(*onexpr_node), head);

					CU_ASSERT_EQUAL(onexpr_node->node_type, AST_TYPE_SEL_ONEXPR);
					CU_ASSERT_FALSE(list_is_empty(&onexpr_node->head));
					CU_ASSERT_EQUAL(list_length(onexpr_node->node_children_head), 1);

					list_for_each(pos3, onexpr_node->node_children_head)
					{
						cmp_node = list_entry(pos3, typeof(*cmp_node), head);

						CU_ASSERT_EQUAL(cmp_node->node_type, AST_TYPE_SEL_CMP);
						CU_ASSERT_FALSE(list_is_empty(&cmp_node->head));
						CU_ASSERT_EQUAL(list_length(cmp_node->node_children_head), 2);
						CU_ASSERT_EQUAL(cmp_node->cmp_type, AST_CMP_EQUALS_OP);

						k = 0;
						list_for_each(pos4, cmp_node->node_children_head)
						{
							field_node = list_entry(pos4, typeof(*field_node),
										head);

							CU_ASSERT_EQUAL(field_node->node_type,
									AST_TYPE_SEL_FIELDNAME);
							CU_ASSERT_FALSE(list_is_empty(&field_node->head));
							CU_ASSERT_EQUAL(list_length(
											field_node->node_children_head),
									0);
							if (k == 0) {

								CU_ASSERT_STRING_EQUAL(field_node->table_name, "t1");
								CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
							} else {
								CU_ASSERT_STRING_EQUAL(field_node->table_name, "t2");
								CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
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

	// optimise it
	CU_ASSERT_EQUAL(optimiser_run(&db, node, &output), MIDORIDB_OK);

//	after
	select_node = (typeof(select_node))node;
	CU_ASSERT_EQUAL(select_node->node_type, AST_TYPE_SEL_SELECT);
	CU_ASSERT_FALSE(select_node->distinct);
	CU_ASSERT_EQUAL(list_length(select_node->node_children_head), 5);
	CU_ASSERT(list_is_empty(&select_node->head));

	i = 0;
	list_for_each(pos1, select_node->node_children_head)
	{
		if (i == 0) {
			field_node = list_entry(pos1, typeof(*field_node), head);

			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);

			CU_ASSERT_STRING_EQUAL(field_node->table_name, "B");
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
		} else if (i == 1) {
			field_node = list_entry(pos1, typeof(*field_node), head);

			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);

			CU_ASSERT_STRING_EQUAL(field_node->table_name, "B");
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "f2");
		} else if (i == 2) {
			field_node = list_entry(pos1, typeof(*field_node), head);

			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);

			CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
		} else if (i == 3) {
			field_node = list_entry(pos1, typeof(*field_node), head);

			CU_ASSERT_EQUAL(field_node->node_type, AST_TYPE_SEL_FIELDNAME);
			CU_ASSERT_FALSE(list_is_empty(&field_node->head));
			CU_ASSERT_EQUAL(list_length(field_node->node_children_head), 0);

			CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
			CU_ASSERT_STRING_EQUAL(field_node->col_name, "f2");
		} else {

			join_node = list_entry(pos1, typeof(*join_node), head);

			CU_ASSERT_EQUAL(join_node->node_type, AST_TYPE_SEL_JOIN);
			CU_ASSERT_FALSE(list_is_empty(&join_node->head));
			CU_ASSERT_EQUAL(list_length(join_node->node_children_head), 3);
			CU_ASSERT_EQUAL(join_node->join_type, AST_SEL_JOIN_INNER);

			j = 0;
			list_for_each(pos2, join_node->node_children_head)
			{
				// order seems different after optimisation...I wonder if I care...
				if (j == 0) {
					table_node = list_entry(pos2, typeof(*table_node), head);

					CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
					CU_ASSERT_FALSE(list_is_empty(&table_node->head));
					CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);

					CU_ASSERT_STRING_EQUAL(table_node->table_name, "A");
				} else if (j == 1) {
					table_node = list_entry(pos2, typeof(*table_node), head);

					CU_ASSERT_EQUAL(table_node->node_type, AST_TYPE_SEL_TABLE);
					CU_ASSERT_FALSE(list_is_empty(&table_node->head));
					CU_ASSERT_EQUAL(list_length(table_node->node_children_head), 0);

					CU_ASSERT_STRING_EQUAL(table_node->table_name, "B");
				} else {
					onexpr_node = list_entry(pos2, typeof(*onexpr_node), head);

					CU_ASSERT_EQUAL(onexpr_node->node_type, AST_TYPE_SEL_ONEXPR);
					CU_ASSERT_FALSE(list_is_empty(&onexpr_node->head));
					CU_ASSERT_EQUAL(list_length(onexpr_node->node_children_head), 1);

					list_for_each(pos3, onexpr_node->node_children_head)
					{
						cmp_node = list_entry(pos3, typeof(*cmp_node), head);

						CU_ASSERT_EQUAL(cmp_node->node_type, AST_TYPE_SEL_CMP);
						CU_ASSERT_FALSE(list_is_empty(&cmp_node->head));
						CU_ASSERT_EQUAL(list_length(cmp_node->node_children_head), 2);
						CU_ASSERT_EQUAL(cmp_node->cmp_type, AST_CMP_EQUALS_OP);

						k = 0;
						list_for_each(pos4, cmp_node->node_children_head)
						{
							field_node = list_entry(pos4, typeof(*field_node),
										head);

							CU_ASSERT_EQUAL(field_node->node_type,
									AST_TYPE_SEL_FIELDNAME);
							CU_ASSERT_FALSE(list_is_empty(&field_node->head));
							CU_ASSERT_EQUAL(list_length(
											field_node->node_children_head),
									0);
							if (k == 0) {

								CU_ASSERT_STRING_EQUAL(field_node->table_name, "A");
								CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
							} else {
								CU_ASSERT_STRING_EQUAL(field_node->table_name, "B");
								CU_ASSERT_STRING_EQUAL(field_node->col_name, "f1");
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

	ast_free(node);
	database_close(&db);
}

void test_optimiser_select(void)
{
	/* exprval (name) */
	select_case_1();

	/* exprval (name) + alias */
	select_case_2();

	/* fieldname - no optimisation expected*/
	select_case_3();

	/* fieldname + table alias */
	select_case_4();

	/* select all */
	select_case_5();

	/* select all + multi-table + table alias (big boss) */
	select_case_6();

}
