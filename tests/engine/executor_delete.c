/*
 * executor_insert.c
 *
 *  Created on: 1/09/2023
 *      Author: paulo
 */

#include <tests/engine.h>
#include <engine/executor.h>

extern struct ast_node* build_ast(char *stmt);

/* delete tests require tables to exist. This helper function deals with the
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

static void test_delete_2(void)
{
	struct fp_types_row {
		int64_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4, *exp_row_5, *exp_row_6;
	struct fp_types_row fp_data_1 = {123};
	struct fp_types_row fp_data_2 = {456};
	struct fp_types_row fp_data_3 = {789};
	struct fp_types_row fp_data_4 = {101112};
	struct fp_types_row fp_data_5 = {-789};
	struct fp_types_row fp_data_6 = {-12345};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);
	exp_row_5 = build_row(&fp_data_5, sizeof(fp_data_5), NULL, 0);
	exp_row_6 = build_row(&fp_data_6, sizeof(fp_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, exp_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 = 123;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, exp_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
	free(exp_row_5);
	free(exp_row_6);
}

static void test_delete_3(void)
{
	struct fp_types_row {
		int64_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4, *exp_row_5, *exp_row_6;
	struct fp_types_row fp_data_1 = {123};
	struct fp_types_row fp_data_2 = {456};
	struct fp_types_row fp_data_3 = {789};
	struct fp_types_row fp_data_4 = {101112};
	struct fp_types_row fp_data_5 = {-789};
	struct fp_types_row fp_data_6 = {-12345};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);
	exp_row_5 = build_row(&fp_data_5, sizeof(fp_data_5), NULL, 0);
	exp_row_6 = build_row(&fp_data_6, sizeof(fp_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, exp_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 > 123;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row_flags(table, 2, &header_deleted));
	CU_ASSERT(check_row_flags(table, 3, &header_deleted));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, exp_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
	free(exp_row_5);
	free(exp_row_6);
}

static void test_delete_4(void)
{
	struct fp_types_row {
		int64_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4, *exp_row_5, *exp_row_6;
	struct fp_types_row fp_data_1 = {123};
	struct fp_types_row fp_data_2 = {456};
	struct fp_types_row fp_data_3 = {789};
	struct fp_types_row fp_data_4 = {101112};
	struct fp_types_row fp_data_5 = {-789};
	struct fp_types_row fp_data_6 = {-12345};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);
	exp_row_5 = build_row(&fp_data_5, sizeof(fp_data_5), NULL, 0);
	exp_row_6 = build_row(&fp_data_6, sizeof(fp_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, exp_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 >= 123;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row_flags(table, 2, &header_deleted));
	CU_ASSERT(check_row_flags(table, 3, &header_deleted));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, exp_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
	free(exp_row_5);
	free(exp_row_6);
}

static void test_delete_5(void)
{
	struct fp_types_row {
		int64_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4, *exp_row_5, *exp_row_6;
	struct fp_types_row fp_data_1 = {123};
	struct fp_types_row fp_data_2 = {456};
	struct fp_types_row fp_data_3 = {789};
	struct fp_types_row fp_data_4 = {101112};
	struct fp_types_row fp_data_5 = {-789};
	struct fp_types_row fp_data_6 = {-12345};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);
	exp_row_5 = build_row(&fp_data_5, sizeof(fp_data_5), NULL, 0);
	exp_row_6 = build_row(&fp_data_6, sizeof(fp_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, exp_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 < 123;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_deleted));
	CU_ASSERT(check_row_flags(table, 5, &header_deleted));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
	free(exp_row_5);
	free(exp_row_6);
}

static void test_delete_6(void)
{
	struct fp_types_row {
		int64_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4, *exp_row_5, *exp_row_6;
	struct fp_types_row fp_data_1 = {123};
	struct fp_types_row fp_data_2 = {456};
	struct fp_types_row fp_data_3 = {789};
	struct fp_types_row fp_data_4 = {101112};
	struct fp_types_row fp_data_5 = {-789};
	struct fp_types_row fp_data_6 = {-12345};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);
	exp_row_5 = build_row(&fp_data_5, sizeof(fp_data_5), NULL, 0);
	exp_row_6 = build_row(&fp_data_6, sizeof(fp_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, exp_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 <= 123;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_deleted));
	CU_ASSERT(check_row_flags(table, 5, &header_deleted));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
	free(exp_row_5);
	free(exp_row_6);
}

static void test_delete_7(void)
{
	struct fp_types_row {
		int64_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4, *exp_row_5, *exp_row_6;
	struct fp_types_row fp_data_1 = {123};
	struct fp_types_row fp_data_2 = {456};
	struct fp_types_row fp_data_3 = {789};
	struct fp_types_row fp_data_4 = {101112};
	struct fp_types_row fp_data_5 = {-789};
	struct fp_types_row fp_data_6 = {-12345};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);
	exp_row_5 = build_row(&fp_data_5, sizeof(fp_data_5), NULL, 0);
	exp_row_6 = build_row(&fp_data_6, sizeof(fp_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, exp_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 <> 123;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row_flags(table, 2, &header_deleted));
	CU_ASSERT(check_row_flags(table, 3, &header_deleted));
	CU_ASSERT(check_row_flags(table, 4, &header_deleted));
	CU_ASSERT(check_row_flags(table, 5, &header_deleted));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
	free(exp_row_5);
	free(exp_row_6);
}

static void test_delete_8(void)
{
	struct fp_types_row {
		double val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4, *exp_row_5, *exp_row_6;
	struct fp_types_row fp_data_1 = {123.0};
	struct fp_types_row fp_data_2 = {456.0};
	struct fp_types_row fp_data_3 = {789.0};
	struct fp_types_row fp_data_4 = {101112.0};
	struct fp_types_row fp_data_5 = {-789.0};
	struct fp_types_row fp_data_6 = {-12345.0};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);
	exp_row_5 = build_row(&fp_data_5, sizeof(fp_data_5), NULL, 0);
	exp_row_6 = build_row(&fp_data_6, sizeof(fp_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DOUBLE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345.0);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, exp_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 = 123.0;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, exp_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
	free(exp_row_5);
	free(exp_row_6);
}

static void test_delete_9(void)
{
	struct fp_types_row {
		double val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4, *exp_row_5, *exp_row_6;
	struct fp_types_row fp_data_1 = {123.0};
	struct fp_types_row fp_data_2 = {456.0};
	struct fp_types_row fp_data_3 = {789.0};
	struct fp_types_row fp_data_4 = {101112.0};
	struct fp_types_row fp_data_5 = {-789.0};
	struct fp_types_row fp_data_6 = {-12345.0};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);
	exp_row_5 = build_row(&fp_data_5, sizeof(fp_data_5), NULL, 0);
	exp_row_6 = build_row(&fp_data_6, sizeof(fp_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DOUBLE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345.0);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, exp_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 > 123.0;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row_flags(table, 2, &header_deleted));
	CU_ASSERT(check_row_flags(table, 3, &header_deleted));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, exp_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
	free(exp_row_5);
	free(exp_row_6);
}

static void test_delete_10(void)
{
	struct fp_types_row {
		double val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4, *exp_row_5, *exp_row_6;
	struct fp_types_row fp_data_1 = {123.0};
	struct fp_types_row fp_data_2 = {456.0};
	struct fp_types_row fp_data_3 = {789.0};
	struct fp_types_row fp_data_4 = {101112.0};
	struct fp_types_row fp_data_5 = {-789.0};
	struct fp_types_row fp_data_6 = {-12345.0};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);
	exp_row_5 = build_row(&fp_data_5, sizeof(fp_data_5), NULL, 0);
	exp_row_6 = build_row(&fp_data_6, sizeof(fp_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DOUBLE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345.0);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, exp_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 >= 123.0;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row_flags(table, 2, &header_deleted));
	CU_ASSERT(check_row_flags(table, 3, &header_deleted));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, exp_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
	free(exp_row_5);
	free(exp_row_6);
}

static void test_delete_11(void)
{
	struct fp_types_row {
		double val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4, *exp_row_5, *exp_row_6;
	struct fp_types_row fp_data_1 = {123.0};
	struct fp_types_row fp_data_2 = {456.0};
	struct fp_types_row fp_data_3 = {789.0};
	struct fp_types_row fp_data_4 = {101112.0};
	struct fp_types_row fp_data_5 = {-789.0};
	struct fp_types_row fp_data_6 = {-12345.0};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);
	exp_row_5 = build_row(&fp_data_5, sizeof(fp_data_5), NULL, 0);
	exp_row_6 = build_row(&fp_data_6, sizeof(fp_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DOUBLE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345.0);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, exp_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 < 123.0;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_deleted));
	CU_ASSERT(check_row_flags(table, 5, &header_deleted));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
	free(exp_row_5);
	free(exp_row_6);
}

static void test_delete_12(void)
{
	struct fp_types_row {
		double val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4, *exp_row_5, *exp_row_6;
	struct fp_types_row fp_data_1 = {123.0};
	struct fp_types_row fp_data_2 = {456.0};
	struct fp_types_row fp_data_3 = {789.0};
	struct fp_types_row fp_data_4 = {101112.0};
	struct fp_types_row fp_data_5 = {-789.0};
	struct fp_types_row fp_data_6 = {-12345.0};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);
	exp_row_5 = build_row(&fp_data_5, sizeof(fp_data_5), NULL, 0);
	exp_row_6 = build_row(&fp_data_6, sizeof(fp_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DOUBLE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345.0);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, exp_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 <= 123.0;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_deleted));
	CU_ASSERT(check_row_flags(table, 5, &header_deleted));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
	free(exp_row_5);
	free(exp_row_6);
}

static void test_delete_13(void)
{
	struct fp_types_row {
		double val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4, *exp_row_5, *exp_row_6;
	struct fp_types_row fp_data_1 = {123.0};
	struct fp_types_row fp_data_2 = {456.0};
	struct fp_types_row fp_data_3 = {789.0};
	struct fp_types_row fp_data_4 = {101112.0};
	struct fp_types_row fp_data_5 = {-789.0};
	struct fp_types_row fp_data_6 = {-12345.0};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);
	exp_row_5 = build_row(&fp_data_5, sizeof(fp_data_5), NULL, 0);
	exp_row_6 = build_row(&fp_data_6, sizeof(fp_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DOUBLE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345.0);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, exp_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 <> 123.0;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row_flags(table, 2, &header_deleted));
	CU_ASSERT(check_row_flags(table, 3, &header_deleted));
	CU_ASSERT(check_row_flags(table, 4, &header_deleted));
	CU_ASSERT(check_row_flags(table, 5, &header_deleted));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
	free(exp_row_5);
	free(exp_row_6);
}

static void test_delete_14(void)
{
	struct fp_types_row {
		bool val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4;
	struct fp_types_row fp_data_1 = {true};
	struct fp_types_row fp_data_2 = {false};
	struct fp_types_row fp_data_3 = {true};
	struct fp_types_row fp_data_4 = {false};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 TINYINT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (TRUE);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (FALSE);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (TRUE);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (FALSE);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 = true;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row_flags(table, 2, &header_deleted));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
}

static void test_delete_15(void)
{
	struct fp_types_row {
		bool val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4;
	struct fp_types_row fp_data_1 = {true};
	struct fp_types_row fp_data_2 = {false};
	struct fp_types_row fp_data_3 = {true};
	struct fp_types_row fp_data_4 = {false};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 TINYINT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (TRUE);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (FALSE);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (TRUE);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (FALSE);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* these statements should match any rows so nothing should change */
	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 > true;"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 >= true;"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 < true;"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 <= true;"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
}

static void test_delete_16(void)
{
	struct fp_types_row {
		bool val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4;
	struct fp_types_row fp_data_1 = {true};
	struct fp_types_row fp_data_2 = {false};
	struct fp_types_row fp_data_3 = {true};
	struct fp_types_row fp_data_4 = {false};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 TINYINT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (TRUE);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (FALSE);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (TRUE);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (FALSE);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 <> false;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row_flags(table, 2, &header_deleted));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
}

static void test_delete_17(void)
{
	struct fp_types_row {
		int64_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4, *exp_row_5, *exp_row_6;
	struct fp_types_row fp_data_1 = {123};
	struct fp_types_row fp_data_2 = {456};
	struct fp_types_row fp_data_3 = {789};
	struct fp_types_row fp_data_4 = {101112};
	struct fp_types_row fp_data_5 = {-789};
	struct fp_types_row fp_data_6 = {-12345};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);
	exp_row_5 = build_row(&fp_data_5, sizeof(fp_data_5), NULL, 0);
	exp_row_6 = build_row(&fp_data_6, sizeof(fp_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, exp_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* no changes are expected after running these statements */
	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 = NULL;"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 != NULL;"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 > NULL;"), ST_ERROR);
	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 >= NULL;"), ST_ERROR);
	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 < NULL;"), ST_ERROR);
	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 <= NULL;"), ST_ERROR);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, exp_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
	free(exp_row_5);
	free(exp_row_6);
}

static void test_delete_18(void)
{
	struct fp_types_row {
		time_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4;
	struct fp_types_row fp_data_1 = {631108800};
	struct fp_types_row fp_data_2 = {662644800};
	struct fp_types_row fp_data_3 = {694180800};
	struct fp_types_row fp_data_4 = {725803200};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DATE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1990-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1991-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1992-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1993-01-01');"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 = '1990-01-01';"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
}

static void test_delete_19(void)
{
	struct fp_types_row {
		time_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4;
	struct fp_types_row fp_data_1 = {631108800};
	struct fp_types_row fp_data_2 = {662644800};
	struct fp_types_row fp_data_3 = {694180800};
	struct fp_types_row fp_data_4 = {725803200};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DATE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1990-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1991-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1992-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1993-01-01');"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 > '1990-01-01';"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row_flags(table, 2, &header_deleted));
	CU_ASSERT(check_row_flags(table, 3, &header_deleted));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
}

static void test_delete_20(void)
{
	struct fp_types_row {
		time_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4;
	struct fp_types_row fp_data_1 = {631108800};
	struct fp_types_row fp_data_2 = {662644800};
	struct fp_types_row fp_data_3 = {694180800};
	struct fp_types_row fp_data_4 = {725803200};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DATE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1990-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1991-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1992-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1993-01-01');"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 >= '1990-01-01';"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row_flags(table, 2, &header_deleted));
	CU_ASSERT(check_row_flags(table, 3, &header_deleted));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
}

static void test_delete_21(void)
{
	struct fp_types_row {
		time_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4;
	struct fp_types_row fp_data_1 = {631108800};
	struct fp_types_row fp_data_2 = {662644800};
	struct fp_types_row fp_data_3 = {694180800};
	struct fp_types_row fp_data_4 = {725803200};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DATE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1990-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1991-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1992-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1993-01-01');"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 < '1991-01-01';"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
}

static void test_delete_22(void)
{
	struct fp_types_row {
		time_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4;
	struct fp_types_row fp_data_1 = {631108800};
	struct fp_types_row fp_data_2 = {662644800};
	struct fp_types_row fp_data_3 = {694180800};
	struct fp_types_row fp_data_4 = {725803200};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DATE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1990-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1991-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1992-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1993-01-01');"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 <= '1992-01-01';"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row_flags(table, 2, &header_deleted));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
}

static void test_delete_23(void)
{
	struct fp_types_row {
		double val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4, *exp_row_5, *exp_row_6;
	struct fp_types_row fp_data_1 = {123.0};
	struct fp_types_row fp_data_2 = {456.0};
	struct fp_types_row fp_data_3 = {789.0};
	struct fp_types_row fp_data_4 = {101112.0};
	struct fp_types_row fp_data_5 = {-789.0};
	struct fp_types_row fp_data_6 = {-12345.0};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);
	exp_row_5 = build_row(&fp_data_5, sizeof(fp_data_5), NULL, 0);
	exp_row_6 = build_row(&fp_data_6, sizeof(fp_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DOUBLE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345.0);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, exp_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 <> 123.0;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row_flags(table, 2, &header_deleted));
	CU_ASSERT(check_row_flags(table, 3, &header_deleted));
	CU_ASSERT(check_row_flags(table, 4, &header_deleted));
	CU_ASSERT(check_row_flags(table, 5, &header_deleted));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
	free(exp_row_5);
	free(exp_row_6);
}

void test_executor_delete(void)
{

	/* no condition */
	test_delete_1();

	/* single condition - int - equals */
	test_delete_2();

	/* single condition - int - gt */
	test_delete_3();

	/* single condition - int - gte */
	test_delete_4();

	/* single condition - int - lt */
	test_delete_5();

	/* single condition - int - lte */
	test_delete_6();

	/* single condition - int - diff */
	test_delete_7();

	/* single condition - double - equals */
	test_delete_8();

	/* single condition - double - gt */
	test_delete_9();

	/* single condition - double - gte */
	test_delete_10();

	/* single condition - double - lt */
	test_delete_11();

	/* single condition - double - lte */
	test_delete_12();

	/* single condition - double - diff */
	test_delete_13();

	/* single condition - bool - equals */
	test_delete_14();

	/* single condition - bool - gt; gte; lt; lte */
	test_delete_15();

	/* single condition - bool - diff */
	test_delete_16();

	/* single condition - field to null comparison - equals; diff; gt; gte; lt; lte */
	test_delete_17();

	/* single condition - date - equals */
	test_delete_18();

	/* single condition - date - gt */
	test_delete_19();

	/* single condition - date - gte */
	test_delete_20();

	/* single condition - date - lt */
	test_delete_21();

	/* single condition - date - lte */
	test_delete_22();

	/* single condition - date - diff */
	test_delete_23();
}
