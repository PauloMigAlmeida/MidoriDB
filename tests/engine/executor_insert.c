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

	query_free(output);

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
		time_t val_7;
	} __packed;

	struct database db = {0};
	struct table *table;

	struct row *exp_row_1;
	struct row *exp_row_2;
	struct fp_types_row fp_data_1 = {123, 456, 123.0, 456.0, true, 1688116260, 1688472000};
	struct fp_types_row fp_data_2 = {-12345, -78965, -12345.0, -78965.0, false, 1688116260, 1688472000};

	exp_row_1 = build_row(&fp_data_1, sizeof(fp_data_1), NULL, 0);
	exp_row_2 = build_row(&fp_data_2, sizeof(fp_data_2), NULL, 0);

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db,
				"CREATE TABLE TEST ("
				"	f1 INT,"
				"	f2 INT,"
				"	f3 DOUBLE,"
				"	f4 DOUBLE,"
				"	f5 TINYINT,"
				"	f6 DATETIME,"
				"	f7 DATE"
				");");
	CU_ASSERT_EQUAL(run_stmt(&db,
					"INSERT INTO TEST VALUES ("
					"	123,"
					"	456,"
					"	123.0,"
					"	456.0,"
					"	TRUE,"
					"	'2023-06-30 21:11:00',"
					"	'2023-07-05'"
					");"),
			ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db,
					"INSERT INTO TEST VALUES ("
					"	-12345,"
					"	-78965,"
					"	-12345.0,"
					"	-78965.0,"
					"	FALSE,"
					"	'2023-06-30 21:11:00',"
					"	'2023-07-05'"
					");"),
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

static void test_insert_3(void)
{
	struct mix_types_row {
		int64_t val_1;
		int64_t val_2;
		double vaddl_3;
	} __packed;

	struct database db = {0};
	struct table *table;

	char *data1_str = "456";
	struct mix_types_row data_1 = {123, (uintptr_t)data1_str, 123.0};

	char *data2_str = zalloc(4);
	int data_2_null_field[] = {1, 2};
	struct mix_types_row data_2 = {123, (uintptr_t)data2_str, 0};

	char *data3_str = zalloc(4);
	int data_3_null_field[] = {1};
	struct mix_types_row data_3 = {123, (uintptr_t)data3_str, 456.0};

	struct row *exp_row_1;
	struct row *exp_row_2;
	struct row *exp_row_3;

	exp_row_1 = build_row(&data_1, sizeof(data_1), NULL, 0);
	exp_row_2 = build_row(&data_2, sizeof(data_2), data_2_null_field, ARR_SIZE(data_2_null_field));
	exp_row_3 = build_row(&data_3, sizeof(data_3), data_3_null_field, ARR_SIZE(data_3_null_field));

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	table = run_create_stmt(&db, "CREATE TABLE TEST (f1 INT, f2 VARCHAR(4), f3 DOUBLE);");
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123, '456', 123.0);"), ST_OK_EXECUTED);
	// explicit nulls
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123, NULL, NULL);"), ST_OK_EXECUTED);
	// implicit nulls
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST (f1) VALUES (123);"), ST_OK_EXECUTED);
	// explicit nulls + reorder columns
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST (f3, f2, f1) VALUES (NULL, NULL, 123);"), ST_OK_EXECUTED);
	// implicit nulls + reorder columns
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST (f3, f1) VALUES (456.0, 123);"), ST_OK_EXECUTED);

	CU_ASSERT(check_row(table, 0, &header_used, exp_row_1));
	CU_ASSERT(check_row(table, 1, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 2, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 3, &header_used, exp_row_2));
	CU_ASSERT(check_row(table, 4, &header_used, exp_row_3));
	CU_ASSERT(check_row_flags(table, 5, &header_empty));

	/* database_close will take care of freeing table reference so free(table) is a noop */
	database_close(&db);
	free(exp_row_1);
	free(exp_row_2);
	free(exp_row_3);
	free(data2_str);
	free(data3_str);
}

void test_executor_insert(void)
{

	/* insert table - no col_names; fixed-precision; single row */
	test_insert_1();

	/* insert table - fixed-precision; single row; NOT NULL cols */
	test_insert_2();

	/* insert table - mixed precision; single row; NULL and NOT NULL */
	test_insert_3();

	/*
	 * TODO insert table - math expressions; fixed-precision; single row; NOT NULL cols; div by NOT 0 allowed (become NULL)
	 *  Paulo: Add examples with both Div by 0 and without it...
	 *	DIV by 0 becomes NULL rationale:
	 *		this isn't something I can validate on the semantic phase because this expressions can become
	 *		complex enough for instance: INSERT INTO X VALUES (2/(2-(1+1)));
	 *
	 *		Alternatively, if I want to keep execution phase free of this type of responsability then I can
	 *		pre-calculate the value on optimisation phase (which isn't a bad idea actually)....
	 */

	//TODO insert table - math expressions; fixed-precision; single row; NULL cols; div by 0 allowed (become NULL)
	// Paulo: Add examples with both Div by 0 and without it...
}
