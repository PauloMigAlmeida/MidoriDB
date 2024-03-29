/*
 * executor_create.c
 *
 *  Created on: 1/09/2023
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

void test_executor_create(void)
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
}
