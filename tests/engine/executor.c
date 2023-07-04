/*
 * executor.c
 *
 *  Created on: 8/06/2023
 *      Author: paulo
 */

#include <tests/engine.h>
#include <engine/executor.h>

extern struct ast_node* build_ast(char *stmt);

static void test_create_1(void)
{
	struct database db = {0};
	struct ast_node *node;
	struct query_output output = {0};
	struct hashtable_value *value;
	struct table *table;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	node = build_ast("CREATE TABLE TEST (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(executor_run(&db, node, &output), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 1);

	value = hashtable_get(db.tables, "TEST", 5);
	CU_ASSERT_PTR_NOT_NULL(value);
	table = *(struct table**)value->content;

	CU_ASSERT_STRING_EQUAL(table->name, "TEST");
	CU_ASSERT_EQUAL(table->column_count, 2);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 0);

	CU_ASSERT_STRING_EQUAL(table->columns[0].name, "f1");
	CU_ASSERT_EQUAL(table->columns[0].type, CT_INTEGER);
	CU_ASSERT_EQUAL(table->columns[0].precision, 8);
	CU_ASSERT_FALSE(table->columns[0].indexed);
	CU_ASSERT(table->columns[0].nullable);
	CU_ASSERT_FALSE(table->columns[0].unique);
	CU_ASSERT_FALSE(table->columns[0].auto_inc);
	CU_ASSERT_FALSE(table->columns[0].primary_key);

	CU_ASSERT_STRING_EQUAL(table->columns[1].name, "f2");
	CU_ASSERT_EQUAL(table->columns[1].type, CT_INTEGER);
	CU_ASSERT_EQUAL(table->columns[1].precision, 8);
	CU_ASSERT_FALSE(table->columns[1].indexed);
	CU_ASSERT(table->columns[1].nullable);
	CU_ASSERT_FALSE(table->columns[1].unique);
	CU_ASSERT_FALSE(table->columns[1].auto_inc);
	CU_ASSERT_FALSE(table->columns[1].primary_key);

	ast_free(node);
	database_close(&db);
}

static void test_create_2(void)
{
	struct database db = {0};
	struct ast_node *node;
	struct query_output output = {0};
	struct hashtable_value *value;
	struct table *table;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	node = build_ast("CREATE TABLE TEST (f1 INT PRIMARY KEY, f2 INT);");
	CU_ASSERT_EQUAL(executor_run(&db, node, &output), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 1);

	value = hashtable_get(db.tables, "TEST", 5);
	CU_ASSERT_PTR_NOT_NULL(value);
	table = *(struct table**)value->content;

	CU_ASSERT_STRING_EQUAL(table->name, "TEST");
	CU_ASSERT_EQUAL(table->column_count, 2);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 0);

	CU_ASSERT_STRING_EQUAL(table->columns[0].name, "f1");
	CU_ASSERT_EQUAL(table->columns[0].type, CT_INTEGER);
	CU_ASSERT_EQUAL(table->columns[0].precision, 8);
	CU_ASSERT_FALSE(table->columns[0].indexed);
	CU_ASSERT_FALSE(table->columns[0].nullable);
	CU_ASSERT(table->columns[0].unique);
	CU_ASSERT_FALSE(table->columns[0].auto_inc);
	CU_ASSERT(table->columns[0].primary_key);

	CU_ASSERT_STRING_EQUAL(table->columns[1].name, "f2");
	CU_ASSERT_EQUAL(table->columns[1].type, CT_INTEGER);
	CU_ASSERT_EQUAL(table->columns[1].precision, 8);
	CU_ASSERT_FALSE(table->columns[1].indexed);
	CU_ASSERT(table->columns[1].nullable);
	CU_ASSERT_FALSE(table->columns[1].unique);
	CU_ASSERT_FALSE(table->columns[1].auto_inc);
	CU_ASSERT_FALSE(table->columns[1].primary_key);

	ast_free(node);
	database_close(&db);
}

static void test_create_3(void)
{
	struct database db = {0};
	struct ast_node *node;
	struct query_output output = {0};
	struct hashtable_value *value;
	struct table *table;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	node = build_ast("CREATE TABLE TEST ("
				"f1 INT AUTO_INCREMENT PRIMARY KEY, "
				"f2 INT NOT NULL,"
				"INDEX(f2));");
	CU_ASSERT_EQUAL(executor_run(&db, node, &output), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 1);

	value = hashtable_get(db.tables, "TEST", 5);
	CU_ASSERT_PTR_NOT_NULL(value);
	table = *(struct table**)value->content;

	CU_ASSERT_STRING_EQUAL(table->name, "TEST");
	CU_ASSERT_EQUAL(table->column_count, 2);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 0);

	CU_ASSERT_STRING_EQUAL(table->columns[0].name, "f1");
	CU_ASSERT_EQUAL(table->columns[0].type, CT_INTEGER);
	CU_ASSERT_EQUAL(table->columns[0].precision, 8);
	CU_ASSERT_FALSE(table->columns[0].indexed);
	CU_ASSERT_FALSE(table->columns[0].nullable);
	CU_ASSERT(table->columns[0].unique);
	CU_ASSERT(table->columns[0].auto_inc);
	CU_ASSERT(table->columns[0].primary_key);

	CU_ASSERT_STRING_EQUAL(table->columns[1].name, "f2");
	CU_ASSERT_EQUAL(table->columns[1].type, CT_INTEGER);
	CU_ASSERT_EQUAL(table->columns[1].precision, 8);
	CU_ASSERT(table->columns[1].indexed);
	CU_ASSERT_FALSE(table->columns[1].nullable);
	CU_ASSERT_FALSE(table->columns[1].unique);
	CU_ASSERT_FALSE(table->columns[1].auto_inc);
	CU_ASSERT_FALSE(table->columns[1].primary_key);

	ast_free(node);
	database_close(&db);
}

static void test_create_4(void)
{
	struct database db = {0};
	struct ast_node *node;
	struct query_output output = {0};
	struct hashtable_value *value;
	struct table *table;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	node = build_ast("CREATE TABLE TEST ("
				"f1 INT AUTO_INCREMENT, "
				"f2 INT NOT NULL,"
				"f3 INT UNIQUE NULL,"
				"PRIMARY KEY (f1),"
				"INDEX(f2));");
	CU_ASSERT_EQUAL(executor_run(&db, node, &output), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 1);

	value = hashtable_get(db.tables, "TEST", 5);
	CU_ASSERT_PTR_NOT_NULL(value);
	table = *(struct table**)value->content;

	CU_ASSERT_STRING_EQUAL(table->name, "TEST");
	CU_ASSERT_EQUAL(table->column_count, 3);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 0);

	CU_ASSERT_STRING_EQUAL(table->columns[0].name, "f1");
	CU_ASSERT_EQUAL(table->columns[0].type, CT_INTEGER);
	CU_ASSERT_EQUAL(table->columns[0].precision, 8);
	CU_ASSERT_FALSE(table->columns[0].indexed);
	CU_ASSERT_FALSE(table->columns[0].nullable);
	CU_ASSERT(table->columns[0].unique);
	CU_ASSERT(table->columns[0].auto_inc);
	CU_ASSERT(table->columns[0].primary_key);

	CU_ASSERT_STRING_EQUAL(table->columns[1].name, "f2");
	CU_ASSERT_EQUAL(table->columns[1].type, CT_INTEGER);
	CU_ASSERT_EQUAL(table->columns[1].precision, 8);
	CU_ASSERT(table->columns[1].indexed);
	CU_ASSERT_FALSE(table->columns[1].nullable);
	CU_ASSERT_FALSE(table->columns[1].unique);
	CU_ASSERT_FALSE(table->columns[1].auto_inc);
	CU_ASSERT_FALSE(table->columns[1].primary_key);

	CU_ASSERT_STRING_EQUAL(table->columns[2].name, "f3");
	CU_ASSERT_EQUAL(table->columns[2].type, CT_INTEGER);
	CU_ASSERT_EQUAL(table->columns[2].precision, 8);
	CU_ASSERT_FALSE(table->columns[2].indexed);
	CU_ASSERT(table->columns[2].nullable);
	CU_ASSERT(table->columns[2].unique);
	CU_ASSERT_FALSE(table->columns[2].auto_inc);
	CU_ASSERT_FALSE(table->columns[2].primary_key);

	ast_free(node);
	database_close(&db);
}

static void test_create_5(void)
{
	struct database db = {0};
	struct ast_node *node;
	struct query_output output = {0};
	struct hashtable_value *value;
	struct table *table;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	node = build_ast("CREATE TABLE TEST ("
				"f1 INTEGER AUTO_INCREMENT PRIMARY KEY, "
				"f2 DOUBLE NOT NULL,"
				"f3 DATE UNIQUE NULL,"
				"f4 DATETIME NULL,"
				"f5 VARCHAR(50) NULL,"
				"INDEX(f2));");
	CU_ASSERT_EQUAL(executor_run(&db, node, &output), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 1);

	value = hashtable_get(db.tables, "TEST", 5);
	CU_ASSERT_PTR_NOT_NULL(value);
	table = *(struct table**)value->content;

	CU_ASSERT_STRING_EQUAL(table->name, "TEST");
	CU_ASSERT_EQUAL(table->column_count, 5);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 0);

	CU_ASSERT_STRING_EQUAL(table->columns[0].name, "f1");
	CU_ASSERT_EQUAL(table->columns[0].type, CT_INTEGER);
	CU_ASSERT_EQUAL(table->columns[0].precision, 8);
	CU_ASSERT_FALSE(table->columns[0].indexed);
	CU_ASSERT_FALSE(table->columns[0].nullable);
	CU_ASSERT(table->columns[0].unique);
	CU_ASSERT(table->columns[0].auto_inc);
	CU_ASSERT(table->columns[0].primary_key);

	CU_ASSERT_STRING_EQUAL(table->columns[1].name, "f2");
	CU_ASSERT_EQUAL(table->columns[1].type, CT_DOUBLE);
	CU_ASSERT_EQUAL(table->columns[1].precision, 8);
	CU_ASSERT(table->columns[1].indexed);
	CU_ASSERT_FALSE(table->columns[1].nullable);
	CU_ASSERT_FALSE(table->columns[1].unique);
	CU_ASSERT_FALSE(table->columns[1].auto_inc);
	CU_ASSERT_FALSE(table->columns[1].primary_key);

	CU_ASSERT_STRING_EQUAL(table->columns[2].name, "f3");
	CU_ASSERT_EQUAL(table->columns[2].type, CT_DATE);
	CU_ASSERT_EQUAL(table->columns[2].precision, 8);
	CU_ASSERT_FALSE(table->columns[2].indexed);
	CU_ASSERT(table->columns[2].nullable);
	CU_ASSERT(table->columns[2].unique);
	CU_ASSERT_FALSE(table->columns[2].auto_inc);
	CU_ASSERT_FALSE(table->columns[2].primary_key);

	CU_ASSERT_STRING_EQUAL(table->columns[3].name, "f4");
	CU_ASSERT_EQUAL(table->columns[3].type, CT_DATETIME);
	CU_ASSERT_EQUAL(table->columns[3].precision, 8);
	CU_ASSERT_FALSE(table->columns[3].indexed);
	CU_ASSERT(table->columns[3].nullable);
	CU_ASSERT_FALSE(table->columns[3].unique);
	CU_ASSERT_FALSE(table->columns[3].auto_inc);
	CU_ASSERT_FALSE(table->columns[3].primary_key);

	CU_ASSERT_STRING_EQUAL(table->columns[4].name, "f5");
	CU_ASSERT_EQUAL(table->columns[4].type, CT_VARCHAR);
	CU_ASSERT_EQUAL(table->columns[4].precision, 50);
	CU_ASSERT_FALSE(table->columns[4].indexed);
	CU_ASSERT(table->columns[4].nullable);
	CU_ASSERT_FALSE(table->columns[4].unique);
	CU_ASSERT_FALSE(table->columns[4].auto_inc);
	CU_ASSERT_FALSE(table->columns[4].primary_key);

	ast_free(node);
	database_close(&db);
}

static void test_create_6(void)
{
	struct database db = {0};
	struct ast_node *node;
	struct query_output output = {0};
	struct hashtable_value *value;
	struct table *table;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	/* create it for the first time */
	node = build_ast("CREATE TABLE IF NOT EXISTS TEST (f1 INT PRIMARY KEY, f2 INT);");
	CU_ASSERT_EQUAL(executor_run(&db, node, &output), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 1);

	value = hashtable_get(db.tables, "TEST", 5);
	CU_ASSERT_PTR_NOT_NULL(value);
	table = *(struct table**)value->content;

	CU_ASSERT_STRING_EQUAL(table->name, "TEST");
	CU_ASSERT_EQUAL(table->column_count, 2);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 0);

	CU_ASSERT_STRING_EQUAL(table->columns[0].name, "f1");
	CU_ASSERT_EQUAL(table->columns[0].type, CT_INTEGER);
	CU_ASSERT_EQUAL(table->columns[0].precision, 8);
	CU_ASSERT_FALSE(table->columns[0].indexed);
	CU_ASSERT_FALSE(table->columns[0].nullable);
	CU_ASSERT(table->columns[0].unique);
	CU_ASSERT_FALSE(table->columns[0].auto_inc);
	CU_ASSERT(table->columns[0].primary_key);

	CU_ASSERT_STRING_EQUAL(table->columns[1].name, "f2");
	CU_ASSERT_EQUAL(table->columns[1].type, CT_INTEGER);
	CU_ASSERT_EQUAL(table->columns[1].precision, 8);
	CU_ASSERT_FALSE(table->columns[1].indexed);
	CU_ASSERT(table->columns[1].nullable);
	CU_ASSERT_FALSE(table->columns[1].unique);
	CU_ASSERT_FALSE(table->columns[1].auto_inc);
	CU_ASSERT_FALSE(table->columns[1].primary_key);

	ast_free(node);

	/* try to create it again - this time nothing should change */
	node = build_ast("CREATE TABLE IF NOT EXISTS TEST (f1 INT PRIMARY KEY, f2 INT);");
	CU_ASSERT_EQUAL(executor_run(&db, node, &output), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 1);

	value = hashtable_get(db.tables, "TEST", 5);
	CU_ASSERT_PTR_NOT_NULL(value);
	table = *(struct table**)value->content;

	CU_ASSERT_STRING_EQUAL(table->name, "TEST");
	CU_ASSERT_EQUAL(table->column_count, 2);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 0);

	CU_ASSERT_STRING_EQUAL(table->columns[0].name, "f1");
	CU_ASSERT_EQUAL(table->columns[0].type, CT_INTEGER);
	CU_ASSERT_EQUAL(table->columns[0].precision, 8);
	CU_ASSERT_FALSE(table->columns[0].indexed);
	CU_ASSERT_FALSE(table->columns[0].nullable);
	CU_ASSERT(table->columns[0].unique);
	CU_ASSERT_FALSE(table->columns[0].auto_inc);
	CU_ASSERT(table->columns[0].primary_key);

	CU_ASSERT_STRING_EQUAL(table->columns[1].name, "f2");
	CU_ASSERT_EQUAL(table->columns[1].type, CT_INTEGER);
	CU_ASSERT_EQUAL(table->columns[1].precision, 8);
	CU_ASSERT_FALSE(table->columns[1].indexed);
	CU_ASSERT(table->columns[1].nullable);
	CU_ASSERT_FALSE(table->columns[1].unique);
	CU_ASSERT_FALSE(table->columns[1].auto_inc);
	CU_ASSERT_FALSE(table->columns[1].primary_key);

	ast_free(node);

	database_close(&db);
}

static void test_create_7(void)
{
	struct database db = {0};
	struct ast_node *node;
	struct query_output output = {0};
	struct hashtable_value *value;
	struct table *table;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	node = build_ast("CREATE TABLE TEST ("
				"f1 INT AUTO_INCREMENT PRIMARY KEY, "
				"f2 INT PRIMARY KEY,"
				"f3 DOUBLE,"
				"f4 DATE NOT NULL,"
				"INDEX(f3,f4));");
	CU_ASSERT_EQUAL(executor_run(&db, node, &output), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 1);

	value = hashtable_get(db.tables, "TEST", 5);
	CU_ASSERT_PTR_NOT_NULL(value);
	table = *(struct table**)value->content;

	CU_ASSERT_STRING_EQUAL(table->name, "TEST");
	CU_ASSERT_EQUAL(table->column_count, 4);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 0);

	CU_ASSERT_STRING_EQUAL(table->columns[0].name, "f1");
	CU_ASSERT_EQUAL(table->columns[0].type, CT_INTEGER);
	CU_ASSERT_EQUAL(table->columns[0].precision, 8);
	CU_ASSERT_FALSE(table->columns[0].indexed);
	CU_ASSERT_FALSE(table->columns[0].nullable);
	CU_ASSERT(table->columns[0].unique);
	CU_ASSERT(table->columns[0].auto_inc);
	CU_ASSERT(table->columns[0].primary_key);

	CU_ASSERT_STRING_EQUAL(table->columns[1].name, "f2");
	CU_ASSERT_EQUAL(table->columns[1].type, CT_INTEGER);
	CU_ASSERT_EQUAL(table->columns[1].precision, 8);
	CU_ASSERT_FALSE(table->columns[1].indexed);
	CU_ASSERT_FALSE(table->columns[1].nullable);
	CU_ASSERT(table->columns[1].unique);
	CU_ASSERT_FALSE(table->columns[1].auto_inc);
	CU_ASSERT(table->columns[1].primary_key);

	CU_ASSERT_STRING_EQUAL(table->columns[2].name, "f3");
	CU_ASSERT_EQUAL(table->columns[2].type, CT_DOUBLE);
	CU_ASSERT_EQUAL(table->columns[2].precision, 8);
	CU_ASSERT(table->columns[2].indexed);
	CU_ASSERT(table->columns[2].nullable);
	CU_ASSERT_FALSE(table->columns[2].unique);
	CU_ASSERT_FALSE(table->columns[2].auto_inc);
	CU_ASSERT_FALSE(table->columns[2].primary_key);

	CU_ASSERT_STRING_EQUAL(table->columns[3].name, "f4");
	CU_ASSERT_EQUAL(table->columns[3].type, CT_DATE);
	CU_ASSERT_EQUAL(table->columns[3].precision, 8);
	CU_ASSERT(table->columns[3].indexed);
	CU_ASSERT_FALSE(table->columns[3].nullable);
	CU_ASSERT_FALSE(table->columns[3].unique);
	CU_ASSERT_FALSE(table->columns[3].auto_inc);
	CU_ASSERT_FALSE(table->columns[3].primary_key);

	ast_free(node);
	database_close(&db);
}

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
		printf(output->error.message);
	}

	free(output);

	return ret;
}

static struct row_header_flags header_used = {.deleted = false, .empty = false};
static struct row_header_flags header_empty = {.deleted = false, .empty = true};

static void test_insert_1(void)
{
	struct fp_types_row {
		int64_t val_1;
		int64_t val_2;
		double val_3;
		double val_4;
		bool val_5;
		time_t val_6;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1;
	struct row *exp_row_2;
	//TODO add test for DATE (you got DATETIME only)
	struct fp_types_row fp_data_1 = {123, 456, 123.0, 456.0, true, 1688116260};
	struct fp_types_row fp_data_2 = {-12345, -78965, -12345.0, -78965.0, false, 1688116260};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 INT,f2 INT,f3 DOUBLE,f4 DOUBLE, f5 TINYINT, f6 DATETIME);");
	CU_ASSERT_EQUAL(run_stmt(
			&db,
			"INSERT INTO TEST VALUES (123, 456, 123.0, 456.0, TRUE, '2023-06-30 21:11:00');"),
			ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(
			&db,
			"INSERT INTO TEST VALUES (-12345, -78965, -12345.0, -78965.0, FALSE, '2023-06-30 21:11:00');"),
			ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row_flags(table, 2, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
}

static void test_insert_2(void)
{
	struct fp_types_row {
		int64_t val_1;
		int64_t val_2;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row;
	struct fp_types_row fp_data = {0, 123};
	int fp_data_null_cols[] = {0};

	exp_row = build_row(&fp_data, sizeof(fp_data), fp_data_null_cols, ARR_SIZE(fp_data_null_cols));

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 INT,f2 INT NOT NULL);");

	/* semantic analysis should complain that '123' is not compatible with INT column type */
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES ('123', 123);"), ST_ERROR);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123);"), ST_ERROR);

	/* execution phase should set null_bitmap bit 1 */
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST (f2) VALUES (123);"), ST_OK_EXECUTED);
	/* explicit NULL */
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST (f1, f2) VALUES (NULL, 123);"), ST_OK_EXECUTED);
	/* different column order */
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST (f2, f1) VALUES (123, NULL);"), ST_OK_EXECUTED);
	/* no opt_column_list column order */
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (NULL, 123);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row));
	CU_ASSERT(check_row_flags(table, 4, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row);
}

void test_executor_run(void)
{
	/* create table - no index; no pk */
	test_create_1();

	/* create table - pk */
	test_create_2();

	/* create table - index; pk; auto increment, not null */
	test_create_3();

	/* create table - index; pk; auto increment, not null; null, unique */
	test_create_4();

	/* create table - index; pk; unique; mixed column types */
	test_create_5();

	/* create table - pk; if not exists */
	test_create_6();

	/* create table - composed pk; multi-field index*/
	test_create_7();

	/* insert table - no col_names; fixed-precision; single row */
	test_insert_1();

	/* insert table - no col_names; fixed-precision; single row; NOT NULL cols */
	test_insert_2();
}
