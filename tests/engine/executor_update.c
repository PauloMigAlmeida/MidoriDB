/*
 * executor_update.c
 *
 *  Created on: 3/09/2023
 *      Author: paulo
 */

#include <tests/engine.h>
#include <engine/executor.h>

extern struct ast_node* build_ast(char *stmt);

/* update tests require tables to exist. This helper function deals with the
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

static void test_update_1(void)
{
	struct fp_types_row {
		int64_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1;
	struct row *bef_row_2;

	struct fp_types_row bef_data_1 = {123};
	struct fp_types_row bef_data_2 = {-12345};

	struct row *aft_row_1;
	struct row *aft_row_2;

	struct fp_types_row aft_data_1 = {42};
	struct fp_types_row aft_data_2 = {42};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row_flags(table, 2, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1=42;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row_flags(table, 2, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(aft_row_1);
	free(aft_row_2);
}

static void test_update_2(void)
{
	struct fp_types_row {
		int64_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4, *bef_row_5, *bef_row_6;
	struct fp_types_row bef_data_1 = {123};
	struct fp_types_row bef_data_2 = {456};
	struct fp_types_row bef_data_3 = {789};
	struct fp_types_row bef_data_4 = {101112};
	struct fp_types_row bef_data_5 = {-789};
	struct fp_types_row bef_data_6 = {-12345};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4, *aft_row_5, *aft_row_6;
	struct fp_types_row aft_data_1 = {42};
	struct fp_types_row aft_data_2 = {456};
	struct fp_types_row aft_data_3 = {789};
	struct fp_types_row aft_data_4 = {101112};
	struct fp_types_row aft_data_5 = {-789};
	struct fp_types_row aft_data_6 = {-12345};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);
	bef_row_5 = build_row(&bef_data_5, sizeof(bef_data_5), NULL, 0);
	bef_row_6 = build_row(&bef_data_6, sizeof(bef_data_6), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);
	aft_row_5 = build_row(&aft_data_5, sizeof(aft_data_5), NULL, 0);
	aft_row_6 = build_row(&aft_data_6, sizeof(aft_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, bef_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, bef_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = 42 WHERE f1 = 123;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, aft_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, aft_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(bef_row_5);
	free(bef_row_6);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
	free(aft_row_5);
	free(aft_row_6);
}

static void test_update_3(void)
{
	struct fp_types_row {
		int64_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4, *bef_row_5, *bef_row_6;
	struct fp_types_row bef_data_1 = {123};
	struct fp_types_row bef_data_2 = {456};
	struct fp_types_row bef_data_3 = {789};
	struct fp_types_row bef_data_4 = {101112};
	struct fp_types_row bef_data_5 = {-789};
	struct fp_types_row bef_data_6 = {-12345};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4, *aft_row_5, *aft_row_6;
	struct fp_types_row aft_data_1 = {123};
	struct fp_types_row aft_data_2 = {42};
	struct fp_types_row aft_data_3 = {42};
	struct fp_types_row aft_data_4 = {42};
	struct fp_types_row aft_data_5 = {-789};
	struct fp_types_row aft_data_6 = {-12345};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);
	bef_row_5 = build_row(&bef_data_5, sizeof(bef_data_5), NULL, 0);
	bef_row_6 = build_row(&bef_data_6, sizeof(bef_data_6), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);
	aft_row_5 = build_row(&aft_data_5, sizeof(aft_data_5), NULL, 0);
	aft_row_6 = build_row(&aft_data_6, sizeof(aft_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, bef_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, bef_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = 42 WHERE f1 > 123;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, aft_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, aft_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(bef_row_5);
	free(bef_row_6);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
	free(aft_row_5);
	free(aft_row_6);
}

static void test_update_4(void)
{
	struct fp_types_row {
		int64_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4, *bef_row_5, *bef_row_6;
	struct fp_types_row bef_data_1 = {123};
	struct fp_types_row bef_data_2 = {456};
	struct fp_types_row bef_data_3 = {789};
	struct fp_types_row bef_data_4 = {101112};
	struct fp_types_row bef_data_5 = {-789};
	struct fp_types_row bef_data_6 = {-12345};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4, *aft_row_5, *aft_row_6;
	struct fp_types_row aft_data_1 = {42};
	struct fp_types_row aft_data_2 = {42};
	struct fp_types_row aft_data_3 = {42};
	struct fp_types_row aft_data_4 = {42};
	struct fp_types_row aft_data_5 = {-789};
	struct fp_types_row aft_data_6 = {-12345};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);
	bef_row_5 = build_row(&bef_data_5, sizeof(bef_data_5), NULL, 0);
	bef_row_6 = build_row(&bef_data_6, sizeof(bef_data_6), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);
	aft_row_5 = build_row(&aft_data_5, sizeof(aft_data_5), NULL, 0);
	aft_row_6 = build_row(&aft_data_6, sizeof(aft_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, bef_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, bef_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = 42 WHERE f1 >= 123;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, aft_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, aft_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(bef_row_5);
	free(bef_row_6);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
	free(aft_row_5);
	free(aft_row_6);
}

static void test_update_5(void)
{
	struct fp_types_row {
		int64_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4, *bef_row_5, *bef_row_6;
	struct fp_types_row bef_data_1 = {123};
	struct fp_types_row bef_data_2 = {456};
	struct fp_types_row bef_data_3 = {789};
	struct fp_types_row bef_data_4 = {101112};
	struct fp_types_row bef_data_5 = {-789};
	struct fp_types_row bef_data_6 = {-12345};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4, *aft_row_5, *aft_row_6;
	struct fp_types_row aft_data_1 = {123};
	struct fp_types_row aft_data_2 = {456};
	struct fp_types_row aft_data_3 = {789};
	struct fp_types_row aft_data_4 = {101112};
	struct fp_types_row aft_data_5 = {42};
	struct fp_types_row aft_data_6 = {42};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);
	bef_row_5 = build_row(&bef_data_5, sizeof(bef_data_5), NULL, 0);
	bef_row_6 = build_row(&bef_data_6, sizeof(bef_data_6), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);
	aft_row_5 = build_row(&aft_data_5, sizeof(aft_data_5), NULL, 0);
	aft_row_6 = build_row(&aft_data_6, sizeof(aft_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, bef_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, bef_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = 42 WHERE f1 < 123;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, aft_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, aft_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(bef_row_5);
	free(bef_row_6);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
	free(aft_row_5);
	free(aft_row_6);
}

static void test_update_6(void)
{
	struct fp_types_row {
		int64_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4, *bef_row_5, *bef_row_6;
	struct fp_types_row bef_data_1 = {123};
	struct fp_types_row bef_data_2 = {456};
	struct fp_types_row bef_data_3 = {789};
	struct fp_types_row bef_data_4 = {101112};
	struct fp_types_row bef_data_5 = {-789};
	struct fp_types_row bef_data_6 = {-12345};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4, *aft_row_5, *aft_row_6;
	struct fp_types_row aft_data_1 = {42};
	struct fp_types_row aft_data_2 = {456};
	struct fp_types_row aft_data_3 = {789};
	struct fp_types_row aft_data_4 = {101112};
	struct fp_types_row aft_data_5 = {42};
	struct fp_types_row aft_data_6 = {42};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);
	bef_row_5 = build_row(&bef_data_5, sizeof(bef_data_5), NULL, 0);
	bef_row_6 = build_row(&bef_data_6, sizeof(bef_data_6), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);
	aft_row_5 = build_row(&aft_data_5, sizeof(aft_data_5), NULL, 0);
	aft_row_6 = build_row(&aft_data_6, sizeof(aft_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, bef_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, bef_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = 42 WHERE f1 <= 123;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, aft_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, aft_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(bef_row_5);
	free(bef_row_6);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
	free(aft_row_5);
	free(aft_row_6);
}

static void test_update_7(void)
{
	struct fp_types_row {
		int64_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4, *bef_row_5, *bef_row_6;
	struct fp_types_row bef_data_1 = {123};
	struct fp_types_row bef_data_2 = {456};
	struct fp_types_row bef_data_3 = {789};
	struct fp_types_row bef_data_4 = {101112};
	struct fp_types_row bef_data_5 = {-789};
	struct fp_types_row bef_data_6 = {-12345};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4, *aft_row_5, *aft_row_6;
	struct fp_types_row aft_data_1 = {123};
	struct fp_types_row aft_data_2 = {42};
	struct fp_types_row aft_data_3 = {42};
	struct fp_types_row aft_data_4 = {42};
	struct fp_types_row aft_data_5 = {42};
	struct fp_types_row aft_data_6 = {42};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);
	bef_row_5 = build_row(&bef_data_5, sizeof(bef_data_5), NULL, 0);
	bef_row_6 = build_row(&bef_data_6, sizeof(bef_data_6), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);
	aft_row_5 = build_row(&aft_data_5, sizeof(aft_data_5), NULL, 0);
	aft_row_6 = build_row(&aft_data_6, sizeof(aft_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, bef_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, bef_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = 42 WHERE f1 <> 123;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, aft_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, aft_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(bef_row_5);
	free(bef_row_6);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
	free(aft_row_5);
	free(aft_row_6);
}

static void test_update_8(void)
{
	struct fp_types_row {
		double val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4, *bef_row_5, *bef_row_6;
	struct fp_types_row bef_data_1 = {123.0};
	struct fp_types_row bef_data_2 = {456.0};
	struct fp_types_row bef_data_3 = {789.0};
	struct fp_types_row bef_data_4 = {101112.0};
	struct fp_types_row bef_data_5 = {-789.0};
	struct fp_types_row bef_data_6 = {-12345.0};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4, *aft_row_5, *aft_row_6;
	struct fp_types_row aft_data_1 = {42.0};
	struct fp_types_row aft_data_2 = {456.0};
	struct fp_types_row aft_data_3 = {789.0};
	struct fp_types_row aft_data_4 = {101112.0};
	struct fp_types_row aft_data_5 = {-789.0};
	struct fp_types_row aft_data_6 = {-12345.0};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);
	bef_row_5 = build_row(&bef_data_5, sizeof(bef_data_5), NULL, 0);
	bef_row_6 = build_row(&bef_data_6, sizeof(bef_data_6), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);
	aft_row_5 = build_row(&aft_data_5, sizeof(aft_data_5), NULL, 0);
	aft_row_6 = build_row(&aft_data_6, sizeof(aft_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DOUBLE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345.0);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, bef_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, bef_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = 42.0 WHERE f1 = 123.0;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, aft_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, aft_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(bef_row_5);
	free(bef_row_6);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
	free(aft_row_5);
	free(aft_row_6);
}

static void test_update_9(void)
{
	struct fp_types_row {
		double val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4, *bef_row_5, *bef_row_6;
	struct fp_types_row bef_data_1 = {123.0};
	struct fp_types_row bef_data_2 = {456.0};
	struct fp_types_row bef_data_3 = {789.0};
	struct fp_types_row bef_data_4 = {101112.0};
	struct fp_types_row bef_data_5 = {-789.0};
	struct fp_types_row bef_data_6 = {-12345.0};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4, *aft_row_5, *aft_row_6;
	struct fp_types_row aft_data_1 = {123.0};
	struct fp_types_row aft_data_2 = {42.0};
	struct fp_types_row aft_data_3 = {42.0};
	struct fp_types_row aft_data_4 = {42.0};
	struct fp_types_row aft_data_5 = {-789.0};
	struct fp_types_row aft_data_6 = {-12345.0};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);
	bef_row_5 = build_row(&bef_data_5, sizeof(bef_data_5), NULL, 0);
	bef_row_6 = build_row(&bef_data_6, sizeof(bef_data_6), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);
	aft_row_5 = build_row(&aft_data_5, sizeof(aft_data_5), NULL, 0);
	aft_row_6 = build_row(&aft_data_6, sizeof(aft_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DOUBLE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345.0);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, bef_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, bef_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = 42.0 WHERE f1 > 123.0;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, aft_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, aft_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(bef_row_5);
	free(bef_row_6);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
	free(aft_row_5);
	free(aft_row_6);
}

static void test_update_10(void)
{
	struct fp_types_row {
		double val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4, *bef_row_5, *bef_row_6;
	struct fp_types_row bef_data_1 = {123.0};
	struct fp_types_row bef_data_2 = {456.0};
	struct fp_types_row bef_data_3 = {789.0};
	struct fp_types_row bef_data_4 = {101112.0};
	struct fp_types_row bef_data_5 = {-789.0};
	struct fp_types_row bef_data_6 = {-12345.0};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4, *aft_row_5, *aft_row_6;
	struct fp_types_row aft_data_1 = {42.0};
	struct fp_types_row aft_data_2 = {42.0};
	struct fp_types_row aft_data_3 = {42.0};
	struct fp_types_row aft_data_4 = {42.0};
	struct fp_types_row aft_data_5 = {-789.0};
	struct fp_types_row aft_data_6 = {-12345.0};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);
	bef_row_5 = build_row(&bef_data_5, sizeof(bef_data_5), NULL, 0);
	bef_row_6 = build_row(&bef_data_6, sizeof(bef_data_6), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);
	aft_row_5 = build_row(&aft_data_5, sizeof(aft_data_5), NULL, 0);
	aft_row_6 = build_row(&aft_data_6, sizeof(aft_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DOUBLE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345.0);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, bef_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, bef_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = 42.0 WHERE f1 >= 123.0;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, aft_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, aft_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(bef_row_5);
	free(bef_row_6);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
	free(aft_row_5);
	free(aft_row_6);
}

static void test_update_11(void)
{
	struct fp_types_row {
		double val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4, *bef_row_5, *bef_row_6;
	struct fp_types_row bef_data_1 = {123.0};
	struct fp_types_row bef_data_2 = {456.0};
	struct fp_types_row bef_data_3 = {789.0};
	struct fp_types_row bef_data_4 = {101112.0};
	struct fp_types_row bef_data_5 = {-789.0};
	struct fp_types_row bef_data_6 = {-12345.0};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4, *aft_row_5, *aft_row_6;
	struct fp_types_row aft_data_1 = {123.0};
	struct fp_types_row aft_data_2 = {456.0};
	struct fp_types_row aft_data_3 = {789.0};
	struct fp_types_row aft_data_4 = {101112.0};
	struct fp_types_row aft_data_5 = {42.0};
	struct fp_types_row aft_data_6 = {42.0};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);
	bef_row_5 = build_row(&bef_data_5, sizeof(bef_data_5), NULL, 0);
	bef_row_6 = build_row(&bef_data_6, sizeof(bef_data_6), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);
	aft_row_5 = build_row(&aft_data_5, sizeof(aft_data_5), NULL, 0);
	aft_row_6 = build_row(&aft_data_6, sizeof(aft_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DOUBLE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345.0);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, bef_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, bef_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = 42.0 WHERE f1 < 123.0;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, aft_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, aft_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(bef_row_5);
	free(bef_row_6);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
	free(aft_row_5);
	free(aft_row_6);
}

static void test_update_12(void)
{
	struct fp_types_row {
		double val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4, *bef_row_5, *bef_row_6;
	struct fp_types_row bef_data_1 = {123.0};
	struct fp_types_row bef_data_2 = {456.0};
	struct fp_types_row bef_data_3 = {789.0};
	struct fp_types_row bef_data_4 = {101112.0};
	struct fp_types_row bef_data_5 = {-789.0};
	struct fp_types_row bef_data_6 = {-12345.0};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4, *aft_row_5, *aft_row_6;
	struct fp_types_row aft_data_1 = {42.0};
	struct fp_types_row aft_data_2 = {456.0};
	struct fp_types_row aft_data_3 = {789.0};
	struct fp_types_row aft_data_4 = {101112.0};
	struct fp_types_row aft_data_5 = {42.0};
	struct fp_types_row aft_data_6 = {42.0};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);
	bef_row_5 = build_row(&bef_data_5, sizeof(bef_data_5), NULL, 0);
	bef_row_6 = build_row(&bef_data_6, sizeof(bef_data_6), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);
	aft_row_5 = build_row(&aft_data_5, sizeof(aft_data_5), NULL, 0);
	aft_row_6 = build_row(&aft_data_6, sizeof(aft_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DOUBLE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345.0);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, bef_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, bef_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = 42.0 WHERE f1 <= 123.0;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, aft_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, aft_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(bef_row_5);
	free(bef_row_6);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
	free(aft_row_5);
	free(aft_row_6);
}

static void test_update_13(void)
{
	struct fp_types_row {
		double val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4, *bef_row_5, *bef_row_6;
	struct fp_types_row bef_data_1 = {123.0};
	struct fp_types_row bef_data_2 = {456.0};
	struct fp_types_row bef_data_3 = {789.0};
	struct fp_types_row bef_data_4 = {101112.0};
	struct fp_types_row bef_data_5 = {-789.0};
	struct fp_types_row bef_data_6 = {-12345.0};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4, *aft_row_5, *aft_row_6;
	struct fp_types_row aft_data_1 = {123.0};
	struct fp_types_row aft_data_2 = {42.0};
	struct fp_types_row aft_data_3 = {42.0};
	struct fp_types_row aft_data_4 = {42.0};
	struct fp_types_row aft_data_5 = {42.0};
	struct fp_types_row aft_data_6 = {42.0};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);
	bef_row_5 = build_row(&bef_data_5, sizeof(bef_data_5), NULL, 0);
	bef_row_6 = build_row(&bef_data_6, sizeof(bef_data_6), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);
	aft_row_5 = build_row(&aft_data_5, sizeof(aft_data_5), NULL, 0);
	aft_row_6 = build_row(&aft_data_6, sizeof(aft_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DOUBLE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789.0);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345.0);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, bef_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, bef_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = 42.0 WHERE f1 <> 123.0;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, aft_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, aft_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(bef_row_5);
	free(bef_row_6);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
	free(aft_row_5);
	free(aft_row_6);
}

static void test_update_14(void)
{
	struct fp_types_row {
		bool val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4;
	struct fp_types_row bef_data_1 = {true};
	struct fp_types_row bef_data_2 = {false};
	struct fp_types_row bef_data_3 = {true};
	struct fp_types_row bef_data_4 = {false};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4;
	struct fp_types_row aft_data_1 = {false};
	struct fp_types_row aft_data_2 = {false};
	struct fp_types_row aft_data_3 = {false};
	struct fp_types_row aft_data_4 = {false};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 TINYINT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (TRUE);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (FALSE);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (TRUE);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (FALSE);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = false WHERE f1 = true;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
}

static void test_update_15(void)
{
	struct fp_types_row {
		bool val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4;
	struct fp_types_row bef_data_1 = {true};
	struct fp_types_row bef_data_2 = {false};
	struct fp_types_row bef_data_3 = {true};
	struct fp_types_row bef_data_4 = {false};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 TINYINT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (TRUE);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (FALSE);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (TRUE);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (FALSE);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* these statements should match any rows so nothing should change */
	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = false WHERE f1 > true;"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = false WHERE f1 >= true;"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = false WHERE f1 < true;"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = false WHERE f1 <= true;"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
}

static void test_update_16(void)
{
	struct fp_types_row {
		bool val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4;
	struct fp_types_row bef_data_1 = {true};
	struct fp_types_row bef_data_2 = {false};
	struct fp_types_row bef_data_3 = {true};
	struct fp_types_row bef_data_4 = {false};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4;
	struct fp_types_row aft_data_1 = {false};
	struct fp_types_row aft_data_2 = {false};
	struct fp_types_row aft_data_3 = {false};
	struct fp_types_row aft_data_4 = {false};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 TINYINT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (TRUE);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (FALSE);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (TRUE);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (FALSE);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = false WHERE f1 <> false;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
}

static void test_update_17(void)
{
	struct fp_types_row {
		int64_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4, *bef_row_5, *bef_row_6;
	struct fp_types_row bef_data_1 = {123};
	struct fp_types_row bef_data_2 = {456};
	struct fp_types_row bef_data_3 = {789};
	struct fp_types_row bef_data_4 = {101112};
	struct fp_types_row bef_data_5 = {-789};
	struct fp_types_row bef_data_6 = {0};
	int bef_data_6_null_field[] = {0};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4, *aft_row_5, *aft_row_6;
	struct fp_types_row aft_data_1 = {123};
	struct fp_types_row aft_data_2 = {456};
	struct fp_types_row aft_data_3 = {789};
	struct fp_types_row aft_data_4 = {101112};
	struct fp_types_row aft_data_5 = {-789};
	struct fp_types_row aft_data_6 = {42};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);
	bef_row_5 = build_row(&bef_data_5, sizeof(bef_data_5), NULL, 0);
	bef_row_6 = build_row(&bef_data_6, sizeof(bef_data_6), bef_data_6_null_field, ARR_SIZE(bef_data_6_null_field));

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);
	aft_row_5 = build_row(&aft_data_5, sizeof(aft_data_5), NULL, 0);
	aft_row_6 = build_row(&aft_data_6, sizeof(aft_data_6), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (456);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (101112);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, bef_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, bef_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* no changes are expected after running these statements */
	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = 42 WHERE f1 = NULL;"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = 42 WHERE f1 != NULL;"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = 42 WHERE f1 > NULL;"), ST_ERROR);
	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = 42 WHERE f1 >= NULL;"), ST_ERROR);
	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = 42 WHERE f1 < NULL;"), ST_ERROR);
	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = 42 WHERE f1 <= NULL;"), ST_ERROR);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, bef_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, bef_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* IS NULL */
	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = 42 WHERE f1 IS NULL;"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row(table, 4, &header_used, aft_row_5));
	CU_ASSERT(check_row(table, 5, &header_used, aft_row_6));
	CU_ASSERT(check_row_flags(table, 6, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(bef_row_5);
	free(bef_row_6);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
	free(aft_row_5);
	free(aft_row_6);
}

static void test_update_18(void)
{
	struct fp_types_row {
		time_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4;
	struct fp_types_row bef_data_1 = {631108800};
	struct fp_types_row bef_data_2 = {662644800};
	struct fp_types_row bef_data_3 = {694180800};
	struct fp_types_row bef_data_4 = {725803200};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4;
	struct fp_types_row aft_data_1 = {725803200};
	struct fp_types_row aft_data_2 = {662644800};
	struct fp_types_row aft_data_3 = {694180800};
	struct fp_types_row aft_data_4 = {725803200};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DATE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1990-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1991-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1992-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1993-01-01');"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = '1993-01-01' WHERE f1 = '1990-01-01';"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
}

static void test_update_19(void)
{
	struct fp_types_row {
		time_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4;
	struct fp_types_row bef_data_1 = {631108800};
	struct fp_types_row bef_data_2 = {662644800};
	struct fp_types_row bef_data_3 = {694180800};
	struct fp_types_row bef_data_4 = {725803200};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4;
	struct fp_types_row aft_data_1 = {631108800};
	struct fp_types_row aft_data_2 = {725803200};
	struct fp_types_row aft_data_3 = {725803200};
	struct fp_types_row aft_data_4 = {725803200};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DATE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1990-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1991-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1992-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1993-01-01');"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = '1993-01-01' WHERE f1 > '1990-01-01';"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
}

static void test_update_20(void)
{
	struct fp_types_row {
		time_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4;
	struct fp_types_row bef_data_1 = {631108800};
	struct fp_types_row bef_data_2 = {662644800};
	struct fp_types_row bef_data_3 = {694180800};
	struct fp_types_row bef_data_4 = {725803200};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4;
	struct fp_types_row aft_data_1 = {725803200};
	struct fp_types_row aft_data_2 = {725803200};
	struct fp_types_row aft_data_3 = {725803200};
	struct fp_types_row aft_data_4 = {725803200};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DATE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1990-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1991-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1992-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1993-01-01');"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = '1993-01-01' WHERE f1 >= '1990-01-01';"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
}

static void test_update_21(void)
{
	struct fp_types_row {
		time_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4;
	struct fp_types_row bef_data_1 = {631108800};
	struct fp_types_row bef_data_2 = {662644800};
	struct fp_types_row bef_data_3 = {694180800};
	struct fp_types_row bef_data_4 = {725803200};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4;
	struct fp_types_row aft_data_1 = {725803200};
	struct fp_types_row aft_data_2 = {662644800};
	struct fp_types_row aft_data_3 = {694180800};
	struct fp_types_row aft_data_4 = {725803200};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DATE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1990-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1991-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1992-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1993-01-01');"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = '1993-01-01' WHERE f1 < '1991-01-01';"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
}

static void test_update_22(void)
{
	struct fp_types_row {
		time_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4;
	struct fp_types_row bef_data_1 = {631108800};
	struct fp_types_row bef_data_2 = {662644800};
	struct fp_types_row bef_data_3 = {694180800};
	struct fp_types_row bef_data_4 = {725803200};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4;
	struct fp_types_row aft_data_1 = {725803200};
	struct fp_types_row aft_data_2 = {725803200};
	struct fp_types_row aft_data_3 = {725803200};
	struct fp_types_row aft_data_4 = {725803200};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DATE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1990-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1991-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1992-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1993-01-01');"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = '1993-01-01' WHERE f1 <= '1992-01-01';"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
}

static void test_update_23(void)
{
	struct fp_types_row {
		time_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4;
	struct fp_types_row bef_data_1 = {631108800};
	struct fp_types_row bef_data_2 = {662644800};
	struct fp_types_row bef_data_3 = {694180800};
	struct fp_types_row bef_data_4 = {725803200};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4;
	struct fp_types_row aft_data_1 = {725803200};
	struct fp_types_row aft_data_2 = {725803200};
	struct fp_types_row aft_data_3 = {694180800};
	struct fp_types_row aft_data_4 = {725803200};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 DATE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1990-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1991-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1992-01-01');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('1993-01-01');"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1 = '1993-01-01' WHERE f1 <> '1992-01-01';"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
}

static void test_update_24(void)
{
	struct mix_types_row {
		int64_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4;
	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4;

	char *bef_data1_str = "123";
	struct mix_types_row bef_data_1 = {(uintptr_t)bef_data1_str};

	char *bef_data2_str = "456";
	struct mix_types_row bef_data_2 = {(uintptr_t)bef_data2_str};

	char *bef_data3_str = zalloc(4);
	int bef_data_3_null_field[] = {0};
	struct mix_types_row bef_data_3 = {(uintptr_t)bef_data3_str};

	char *bef_data4_str = "789";
	struct mix_types_row bef_data_4 = {(uintptr_t)bef_data4_str};

	char *aft_data1_str = "852";
	struct mix_types_row aft_data_1 = {(uintptr_t)aft_data1_str};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), bef_data_3_null_field, ARR_SIZE(bef_data_3_null_field));
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	aft_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), bef_data_3_null_field, ARR_SIZE(bef_data_3_null_field));
	aft_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 VARCHAR(4));");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('123');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('456');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (NULL);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('789');"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1='852' WHERE f1 = '123';"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(bef_data3_str);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
}

static void test_update_25(void)
{
	struct mix_types_row {
		int64_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4;
	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4;

	char *bef_data1_str = "123";
	struct mix_types_row bef_data_1 = {(uintptr_t)bef_data1_str};

	char *bef_data2_str = "456";
	struct mix_types_row bef_data_2 = {(uintptr_t)bef_data2_str};

	char *bef_data3_str = zalloc(4);
	int bef_data_3_null_field[] = {0};
	struct mix_types_row bef_data_3 = {(uintptr_t)bef_data3_str};

	char *bef_data4_str = "789";
	struct mix_types_row bef_data_4 = {(uintptr_t)bef_data4_str};

	char *aft_data1_str = "852";
	struct mix_types_row aft_data_1 = {(uintptr_t)aft_data1_str};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), bef_data_3_null_field, ARR_SIZE(bef_data_3_null_field));
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	aft_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), bef_data_3_null_field, ARR_SIZE(bef_data_3_null_field));
	aft_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 VARCHAR(4));");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('123');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('456');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (NULL);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('789');"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* semantic phase should complain about those */
	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1='852' WHERE f1 > '123';"), ST_ERROR);
	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1='852' WHERE f1 >= '456';"), ST_ERROR);
	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1='852' WHERE f1 < NULL;"), ST_ERROR);
	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1='852' WHERE f1 <= '789';"), ST_ERROR);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(bef_data3_str);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
}

static void test_update_26(void)
{
	struct mix_types_row {
		int64_t val_1;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4;
	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4;

	char *bef_data1_str = "123";
	struct mix_types_row bef_data_1 = {(uintptr_t)bef_data1_str};

	char *bef_data2_str = "456";
	struct mix_types_row bef_data_2 = {(uintptr_t)bef_data2_str};

	char *bef_data3_str = zalloc(4);
	int bef_data_3_null_field[] = {0};
	struct mix_types_row bef_data_3 = {(uintptr_t)bef_data3_str};

	char *bef_data4_str = "789";
	struct mix_types_row bef_data_4 = {(uintptr_t)bef_data4_str};

	char *aft_data1_str = "852";
	struct mix_types_row aft_data_1 = {(uintptr_t)aft_data1_str};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), bef_data_3_null_field, ARR_SIZE(bef_data_3_null_field));
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), NULL, 0);

	aft_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), bef_data_3_null_field, ARR_SIZE(bef_data_3_null_field));
	aft_row_4 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 VARCHAR(4));");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('123');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('456');"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (NULL);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('789');"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE TEST SET f1='852' WHERE f1 <> '123';"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(bef_data3_str);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
}

static void test_update_27(void)
{
	struct fp_types_row {
		int64_t val_1;
		int64_t val_2;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4;
	struct fp_types_row bef_data_1 = {123, 123};
	struct fp_types_row bef_data_2 = {456, 123};
	struct fp_types_row bef_data_3 = {789, 987};
	struct fp_types_row bef_data_4 = {101112, 0};
	int bef_data_4_null_field[] = {1};

	struct row *aftc1_row_1, *aftc1_row_2, *aftc1_row_3, *aftc1_row_4;
	struct fp_types_row aftc1_data_1 = {42, 43};
	struct fp_types_row aftc1_data_2 = {456, 123};
	struct fp_types_row aftc1_data_3 = {789, 987};
	struct fp_types_row aftc1_data_4 = {101112, 0};
	int aftc1_data_4_null_field[] = {1};

	struct row *aftc2_row_1, *aftc2_row_2, *aftc2_row_3, *aftc2_row_4;
	struct fp_types_row aftc2_data_1 = {123, 123};
	struct fp_types_row aftc2_data_2 = {42, 43};
	struct fp_types_row aftc2_data_3 = {789, 987};
	struct fp_types_row aftc2_data_4 = {101112, 0};
	int aftc2_data_4_null_field[] = {1};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), bef_data_4_null_field, ARR_SIZE(bef_data_4_null_field));

	aftc1_row_1 = build_row(&aftc1_data_1, sizeof(aftc1_data_1), NULL, 0);
	aftc1_row_2 = build_row(&aftc1_data_2, sizeof(aftc1_data_2), NULL, 0);
	aftc1_row_3 = build_row(&aftc1_data_3, sizeof(aftc1_data_3), NULL, 0);
	aftc1_row_4 = build_row(&aftc1_data_4, sizeof(aftc1_data_4), aftc1_data_4_null_field,
				ARR_SIZE(aftc1_data_4_null_field));

	aftc2_row_1 = build_row(&aftc2_data_1, sizeof(aftc2_data_1), NULL, 0);
	aftc2_row_2 = build_row(&aftc2_data_2, sizeof(aftc2_data_2), NULL, 0);
	aftc2_row_3 = build_row(&aftc2_data_3, sizeof(aftc2_data_3), NULL, 0);
	aftc2_row_4 = build_row(&aftc2_data_4, sizeof(aftc2_data_4), aftc2_data_4_null_field,
				ARR_SIZE(aftc2_data_4_null_field));

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	/* Equals */
	table = run_create_stmt(&db, "CREATE TABLE A (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE A SET f1=42, f2=43 WHERE f1 = f2;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aftc1_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aftc1_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aftc1_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aftc1_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* GT */
	table = run_create_stmt(&db, "CREATE TABLE B (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE B SET f1=42, f2=43 WHERE f1 > f2;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aftc2_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aftc2_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aftc2_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aftc2_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(aftc1_row_1);
	free(aftc1_row_2);
	free(aftc1_row_3);
	free(aftc1_row_4);
	free(aftc2_row_1);
	free(aftc2_row_2);
	free(aftc2_row_3);
	free(aftc2_row_4);
}

static void test_update_28(void)
{
	struct fp_types_row {
		int64_t val_1;
		int64_t val_2;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4;
	struct fp_types_row bef_data_1 = {123, 123};
	struct fp_types_row bef_data_2 = {456, 123};
	struct fp_types_row bef_data_3 = {789, 987};
	struct fp_types_row bef_data_4 = {101112, 0};
	int bef_data_4_null_field[] = {1};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4;
	struct fp_types_row aft_data_1 = {42, 43};
	struct fp_types_row aft_data_2 = {42, 43};
	struct fp_types_row aft_data_3 = {42, 43};
	struct fp_types_row aft_data_4 = {42, 43};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), bef_data_4_null_field, ARR_SIZE(bef_data_4_null_field));

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	/* Equals */
	table = run_create_stmt(&db, "CREATE TABLE A (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE A SET f1=42, f2=43WHERE 1 = 1;"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
}

static void test_update_29(void)
{
	struct fp_types_row {
		int64_t val_1;
		int64_t val_2;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4;
	struct fp_types_row bef_data_1 = {123, 123};
	struct fp_types_row bef_data_2 = {456, 123};
	struct fp_types_row bef_data_3 = {789, 987};
	struct fp_types_row bef_data_4 = {101112, 0};
	int bef_data_4_null_field[] = {1};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4;
	struct fp_types_row aft_data_1 = {123, 123};
	struct fp_types_row aft_data_2 = {456, 123};
	struct fp_types_row aft_data_3 = {789, 987};
	struct fp_types_row aft_data_4 = {42, 43};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), bef_data_4_null_field, ARR_SIZE(bef_data_4_null_field));

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	/* AND + OR + XOR */
	table = run_create_stmt(&db, "CREATE TABLE E (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO E VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO E VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO E VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO E VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(
			&db, "UPDATE E SET f1=42, f2=43 WHERE (f2 < 1000 AND f2 > 100) XOR (f1 > 100 OR f1 > 10000);"),
			ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
}

static void test_update_30(void)
{
	struct fp_types_row {
		int64_t val_1;
		int64_t val_2;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *bef_row_1, *bef_row_2, *bef_row_3, *bef_row_4;
	struct fp_types_row bef_data_1 = {123, 123};
	struct fp_types_row bef_data_2 = {456, 123};
	struct fp_types_row bef_data_3 = {789, 987};
	struct fp_types_row bef_data_4 = {101112, 0};
	int bef_data_4_null_field[] = {1};

	struct row *aft_row_1, *aft_row_2, *aft_row_3, *aft_row_4;
	struct fp_types_row aft_data_1 = {123, 123};
	struct fp_types_row aft_data_2 = {456, 123};
	struct fp_types_row aft_data_3 = {42, 43};
	struct fp_types_row aft_data_4 = {101112, 0};
	int aft_data_4_null_field[] = {1};

	bef_row_1 = build_row(&bef_data_1, sizeof(bef_data_1), NULL, 0);
	bef_row_2 = build_row(&bef_data_2, sizeof(bef_data_2), NULL, 0);
	bef_row_3 = build_row(&bef_data_3, sizeof(bef_data_3), NULL, 0);
	bef_row_4 = build_row(&bef_data_4, sizeof(bef_data_4), bef_data_4_null_field, ARR_SIZE(bef_data_4_null_field));

	aft_row_1 = build_row(&aft_data_1, sizeof(aft_data_1), NULL, 0);
	aft_row_2 = build_row(&aft_data_2, sizeof(aft_data_2), NULL, 0);
	aft_row_3 = build_row(&aft_data_3, sizeof(aft_data_3), NULL, 0);
	aft_row_4 = build_row(&aft_data_4, sizeof(aft_data_4), aft_data_4_null_field, ARR_SIZE(aft_data_4_null_field));

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	/* NOT IN - multiple condition */
	table = run_create_stmt(&db, "CREATE TABLE C (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO C VALUES (123, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO C VALUES (456, 123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO C VALUES (789, 987);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO C VALUES (101112, NULL);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, bef_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, bef_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, bef_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, bef_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	CU_ASSERT_EQUAL(run_stmt(&db, "UPDATE C SET f1=42, f2=43 WHERE f1 IN (456, 789) AND f2 NOT IN (123);"), ST_OK_EXECUTED);
	CU_ASSERT(check_row(table, 0, &header_used, aft_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, aft_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, aft_row_3));
	CU_ASSERT(check_row(table, 3, &header_used, aft_row_4));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(bef_row_1);
	free(bef_row_2);
	free(bef_row_3);
	free(bef_row_4);
	free(aft_row_1);
	free(aft_row_2);
	free(aft_row_3);
	free(aft_row_4);
}

void test_executor_update(void)
{

	/* no condition */
	test_update_1();

	/* single condition - int - equals */
	test_update_2();

	/* single condition - int - gt */
	test_update_3();

	/* single condition - int - gte */
	test_update_4();

	/* single condition - int - lt */
	test_update_5();

	/* single condition - int - lte */
	test_update_6();

	/* single condition - int - diff */
	test_update_7();

	/* single condition - double - equals */
	test_update_8();

	/* single condition - double - gt */
	test_update_9();

	/* single condition - double - gte */
	test_update_10();

	/* single condition - double - lt */
	test_update_11();

	/* single condition - double - lte */
	test_update_12();

	/* single condition - double - diff */
	test_update_13();

	/* single condition - bool - equals */
	test_update_14();

	/* single condition - bool - gt; gte; lt; lte */
	test_update_15();

	/* single condition - bool - diff */
	test_update_16();

	/* single condition - field to null comparison - equals; diff; gt; gte; lt; lte */
	test_update_17();

	/* single condition - date - equals */
	test_update_18();

	/* single condition - date - gt */
	test_update_19();

	/* single condition - date - gte */
	test_update_20();

	/* single condition - date - lt */
	test_update_21();

	/* single condition - date - lte */
	test_update_22();

	/* single condition - date - diff */
	test_update_23();

	/* single condition - str - equals */
	test_update_24();

	/* single condition - str - gt; gte; lt; lte */
	test_update_25();

	/* single condition - str - diff */
	test_update_26();

	/* single condition - field to field */
	test_update_27();

	/* single condition - value to value */
	test_update_28();

	/* multiple condition - logical operators */
	test_update_29();

	/* multiple condition - IN / NOT IN*/
	test_update_30();
}
