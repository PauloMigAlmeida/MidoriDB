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

	query_free(output);

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
	struct fp_types_row fp_data_6 = {0};

	int data_6_null_field[] = {0};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), NULL, 0);
	exp_row_5 = build_row(&fp_data_5, sizeof(fp_data_5), NULL, 0);
	exp_row_6 = build_row(&fp_data_6, sizeof(fp_data_6), data_6_null_field, ARR_SIZE(data_6_null_field));

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (NULL);"), ST_OK_EXECUTED);

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

	/* IS NULL */
	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 IS NULL;"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_5));
	CU_ASSERT(check_row_flags(table, 5, &header_deleted));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* IS NOT NULL */
	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 IS NOT NULL;"), ST_OK_EXECUTED);

	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
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

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 <> '1992-01-01';"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row_flags(table, 3, &header_deleted));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
}

static void test_delete_24(void)
{
	struct mix_types_row {
		int64_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4;

	char *data1_str = "123";
	struct mix_types_row data_1 = {(uintptr_t)data1_str};

	char *data2_str = "456";
	struct mix_types_row data_2 = {(uintptr_t)data2_str};

	char *data3_str = zalloc(4);
	int data_3_null_field[] = {0};
	struct mix_types_row data_3 = {(uintptr_t)data3_str};

	char *data4_str = "789";
	struct mix_types_row data_4 = {(uintptr_t)data4_str};

	exp_row_1 = build_row(&data_1, sizeof(data_1), NULL, 0);
	exp_row_2 = build_row(&data_2, sizeof(data_2), NULL, 0);
	exp_row_3 = build_row(&data_3, sizeof(data_3), data_3_null_field, ARR_SIZE(data_3_null_field));
	exp_row_4 = build_row(&data_4, sizeof(data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 VARCHAR(4));");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('123');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('456');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (NULL);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('789');"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 = '123';"), ST_OK_EXECUTED);
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
	free(data3_str);
}

static void test_delete_25(void)
{
	struct mix_types_row {
		int64_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4;

	char *data1_str = "123";
	struct mix_types_row data_1 = {(uintptr_t)data1_str};

	char *data2_str = "456";
	struct mix_types_row data_2 = {(uintptr_t)data2_str};

	char *data3_str = zalloc(4);
	int data_3_null_field[] = {0};
	struct mix_types_row data_3 = {(uintptr_t)data3_str};

	char *data4_str = "789";
	struct mix_types_row data_4 = {(uintptr_t)data4_str};

	exp_row_1 = build_row(&data_1, sizeof(data_1), NULL, 0);
	exp_row_2 = build_row(&data_2, sizeof(data_2), NULL, 0);
	exp_row_3 = build_row(&data_3, sizeof(data_3), data_3_null_field, ARR_SIZE(data_3_null_field));
	exp_row_4 = build_row(&data_4, sizeof(data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 VARCHAR(4));");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('123');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('456');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (NULL);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('789');"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* semantic phase should complain about those */
	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 > '123';"), ST_ERROR);
	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 >= '456';"), ST_ERROR);
	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 < NULL;"), ST_ERROR);
	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 <= '789';"), ST_ERROR);

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
	free(data3_str);
}

static void test_delete_26(void)
{
	struct mix_types_row {
		int64_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4;

	char *data1_str = "123";
	struct mix_types_row data_1 = {(uintptr_t)data1_str};

	char *data2_str = "456";
	struct mix_types_row data_2 = {(uintptr_t)data2_str};

	char *data3_str = zalloc(4);
	int data_3_null_field[] = {0};
	struct mix_types_row data_3 = {(uintptr_t)data3_str};

	char *data4_str = "789";
	struct mix_types_row data_4 = {(uintptr_t)data4_str};

	exp_row_1 = build_row(&data_1, sizeof(data_1), NULL, 0);
	exp_row_2 = build_row(&data_2, sizeof(data_2), NULL, 0);
	exp_row_3 = build_row(&data_3, sizeof(data_3), data_3_null_field, ARR_SIZE(data_3_null_field));
	exp_row_4 = build_row(&data_4, sizeof(data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 VARCHAR(4));");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('123');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('456');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (NULL);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('789');"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM TEST WHERE f1 <> '123';"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3)); // NULL doesn't evaluate to true using CMP
	CU_ASSERT(check_row_flags(table, 3, &header_deleted))
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
	free(data3_str);
}

static void test_delete_27(void)
{
	struct fp_types_row {
		int64_t val_1;
		int64_t val_2;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4;
	struct fp_types_row fp_data_1 = {123, 123};
	struct fp_types_row fp_data_2 = {456, 123};
	struct fp_types_row fp_data_3 = {789, 987};
	struct fp_types_row fp_data_4 = {101112, 0};

	int data_4_null_field[] = {1};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), data_4_null_field, ARR_SIZE(data_4_null_field));

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	/* Equals */
	table = run_create_stmt(&db, "CREATE TABLE A (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM A WHERE f1 = f2;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* GT */
	table = run_create_stmt(&db, "CREATE TABLE B (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM B WHERE f1 > f2;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* GTE */
	table = run_create_stmt(&db, "CREATE TABLE C (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO C VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO C VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO C VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO C VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM C WHERE f1 >= f2;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* LT */
	table = run_create_stmt(&db, "CREATE TABLE D (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO D VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO D VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO D VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO D VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM D WHERE f1 < f2;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row_flags(table, 2, &header_deleted));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* LTE */
	table = run_create_stmt(&db, "CREATE TABLE E (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO E VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO E VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO E VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO E VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM E WHERE f1 <= f2;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row_flags(table, 2, &header_deleted));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* Diff */
	table = run_create_stmt(&db, "CREATE TABLE F (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO F VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO F VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO F VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO F VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM F WHERE f1 <> f2;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row_flags(table, 2, &header_deleted));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4)); // NULL doesn't evaluate to true using CMP
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* NULL CMP - no effect expected */
	table = run_create_stmt(&db, "CREATE TABLE G (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO G VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO G VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO G VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO G VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM G WHERE f1 = NULL;"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM G WHERE f1 <> NULL;"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM G WHERE NULL = f1;"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM G WHERE NULL <> f1;"), ST_OK_EXECUTED);

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

static void test_delete_28(void)
{
	struct fp_types_row {
		int64_t val_1;
		int64_t val_2;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4;
	struct fp_types_row fp_data_1 = {123, 123};
	struct fp_types_row fp_data_2 = {456, 123};
	struct fp_types_row fp_data_3 = {789, 987};
	struct fp_types_row fp_data_4 = {101112, 0};

	int data_4_null_field[] = {1};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), data_4_null_field, ARR_SIZE(data_4_null_field));

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	/* Equals */
	table = run_create_stmt(&db, "CREATE TABLE A (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM A WHERE 1 = 1;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row_flags(table, 2, &header_deleted));
	CU_ASSERT(check_row_flags(table, 3, &header_deleted));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* GT */
	table = run_create_stmt(&db, "CREATE TABLE B (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM B WHERE 1 > 1;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* GTE */
	table = run_create_stmt(&db, "CREATE TABLE C (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO C VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO C VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO C VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO C VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM C WHERE 1 >= 2;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* LT */
	table = run_create_stmt(&db, "CREATE TABLE D (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO D VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO D VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO D VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO D VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM D WHERE 1 < 2;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row_flags(table, 2, &header_deleted));
	CU_ASSERT(check_row_flags(table, 3, &header_deleted));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* LTE */
	table = run_create_stmt(&db, "CREATE TABLE E (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO E VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO E VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO E VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO E VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM E WHERE 1 <= 2;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row_flags(table, 2, &header_deleted));
	CU_ASSERT(check_row_flags(table, 3, &header_deleted));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* Diff */
	table = run_create_stmt(&db, "CREATE TABLE F (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO F VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO F VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO F VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO F VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM F WHERE 1 <> 1;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* NULL CMP - no effect expected */
	table = run_create_stmt(&db, "CREATE TABLE G (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO G VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO G VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO G VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO G VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM G WHERE 1 = NULL;"), ST_ERROR);
	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM G WHERE 1 <> NULL;"), ST_ERROR);
	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM G WHERE NULL = 1;"), ST_ERROR);
	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM G WHERE NULL <> 1;"), ST_ERROR);

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

static void test_delete_29(void)
{
	struct fp_types_row {
		int64_t val_1;
		int64_t val_2;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4;
	struct fp_types_row fp_data_1 = {123, 123};
	struct fp_types_row fp_data_2 = {456, 123};
	struct fp_types_row fp_data_3 = {789, 987};
	struct fp_types_row fp_data_4 = {101112, 0};

	int data_4_null_field[] = {1};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), data_4_null_field, ARR_SIZE(data_4_null_field));

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	/* OR */
	table = run_create_stmt(&db, "CREATE TABLE A (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM A WHERE f1 = 150 OR 1 = 1;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row_flags(table, 2, &header_deleted));
	CU_ASSERT(check_row_flags(table, 3, &header_deleted));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* AND */
	table = run_create_stmt(&db, "CREATE TABLE B (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM B WHERE f1 = 150 AND 1 = 1;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* XOR */
	table = run_create_stmt(&db, "CREATE TABLE C (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO C VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO C VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO C VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO C VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM C WHERE f1 > 0 XOR f2 > 100;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row_flags(table, 3, &header_deleted)); // NULL evaluates to false
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* AND + OR */
	table = run_create_stmt(&db, "CREATE TABLE D (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO D VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO D VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO D VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO D VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM D WHERE f1 > 100 AND f1 < 500 OR f2 is NULL;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row_flags(table, 3, &header_deleted));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* AND + OR + XOR */
	table = run_create_stmt(&db, "CREATE TABLE E (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO E VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO E VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO E VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO E VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM E WHERE (f2 < 1000 AND f2 > 100) XOR (f1 > 100 OR f1 > 10000);"),
			ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row_flags(table, 3, &header_deleted));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(exp_row_4);
}

static void test_delete_30(void)
{
	struct fp_types_row {
		int64_t val_1;
		int64_t val_2;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1, *exp_row_2, *exp_row_3, *exp_row_4;
	struct fp_types_row fp_data_1 = {123, 123};
	struct fp_types_row fp_data_2 = {456, 123};
	struct fp_types_row fp_data_3 = {789, 987};
	struct fp_types_row fp_data_4 = {101112, 0};

	int data_4_null_field[] = {1};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);
	exp_row_3 = build_row(&fp_data_3, sizeof(fp_data_3), NULL, 0);
	exp_row_4 = build_row(&fp_data_4, sizeof(fp_data_4), data_4_null_field, ARR_SIZE(data_4_null_field));

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	/* Equals */
	table = run_create_stmt(&db, "CREATE TABLE A (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM A WHERE f1 in (123, 456);"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* IN - multiple condition */
	table = run_create_stmt(&db, "CREATE TABLE B (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM B WHERE f1 in (123, 456) OR f2 in (NULL);"), ST_OK_EXECUTED);
	CU_ASSERT(check_row_flags(table, 0, &header_deleted));
	CU_ASSERT(check_row_flags(table, 1, &header_deleted));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4)); // NULL should have not effect here
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* NOT IN - multiple condition */
	table = run_create_stmt(&db, "CREATE TABLE C (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO C VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO C VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO C VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO C VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "DELETE FROM C WHERE f1 IN (456, 789) AND f2 NOT IN (123);"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
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

	/* single condition - str - equals */
	test_delete_24();

	/* single condition - str - gt; gte; lt; lte */
	test_delete_25();

	/* single condition - str - diff */
	test_delete_26();

	/* single condition - field to field */
	test_delete_27();

	/* single condition - value to value */
	test_delete_28();

	/* multiple condition - logical operators */
	test_delete_29();

	/* multiple condition - IN / NOT IN*/
	test_delete_30();
}
