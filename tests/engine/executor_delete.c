/*
 * executor_insert.c
 *
 *  Created on: 1/09/2023
 *      Author: paulo
 */

#include <tests/engine.h>
#include <engine/executor.h>

extern struct ast_node* build_ast(char *stmt);

/* insert tests require tables to exist. This helper function deals with the
 * cumbersome tasks of executing the queries. It assumes that the database is
 * already opened
 */
static struct table* run_create_stmt(struct database *db, char *stmt)
{
	struct table *table;
	struct ast_node *node;
	struct ast_crt_create_node *create_node;
	struct query_output output = {0};
	struct hashtable_value *value;
	int table_count;

	// test if an additional table has been added to database
	table_count = db->tables->count;
	node = build_ast(stmt);
	CU_ASSERT_EQUAL_FATAL(executor_run(db, node, &output), MIDORIDB_OK);
	CU_ASSERT_EQUAL_FATAL(db->tables->count, table_count + 1);

	CU_ASSERT_EQUAL_FATAL(node->node_type, AST_TYPE_CRT_CREATE);
	create_node = (typeof(create_node))node;

	// fetch table from database schema
	value = hashtable_get(db->tables, create_node->table_name, strlen(create_node->table_name) + 1);
	CU_ASSERT_PTR_NOT_NULL(value);
	table = *(struct table**)value->content;

	// test if table created is empty as expected
	CU_ASSERT_STRING_EQUAL(table->name, create_node->table_name);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 0);
	CU_ASSERT_EQUAL(count_datablocks(table), 0);

	ast_free(node);
	return table;
}

static enum query_output_status run_stmt(struct database *db, char *stmt)
{
	struct query_output *output;
	enum query_output_status ret;

	output = query_execute(db, stmt);
	CU_ASSERT_PTR_NOT_NULL_FATAL(output);
	ret = output->status;

	if (ret == ST_ERROR) {
		// helps diagnose issues during unit tests / CI builds
		printf("%s\n", output->error.message);
	}

	free(output);

	return ret;
}

static struct row_header_flags header_used = {.deleted = false, .empty = false};
static struct row_header_flags header_empty = {.deleted = false, .empty = true};
static struct row_header_flags header_deleted = {.deleted = true, .empty = false};

static void test_delete_1(void)
{
	struct fp_types_row {
		int64_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1;
	struct row *exp_row_2;
	struct fp_types_row fp_data_1 = {123};
	struct fp_types_row fp_data_2 = {-12345};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row_flags(table, 2, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row_flags(table, 2, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
}

void test_executor_delete(void)
{

	/* no condition */
	test_delete_1();

}
