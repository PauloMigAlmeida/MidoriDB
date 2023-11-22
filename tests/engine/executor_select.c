/*
 * executor_select.c
 *
 *  Created on: 15/11/2023
 *      Author: paulo
 */

#include <tests/engine.h>
#include <engine/executor.h>

static enum query_output_status run_stmt(struct database *db, char *stmt)
{
	struct query_output *output;
	enum query_output_status ret;

	output = query_execute(db, stmt);
	CU_ASSERT_PTR_NOT_NULL_FATAL(output);
	ret = output->status;

	// helps diagnose issues during unit tests / CI builds
	if (ret != ST_OK_EXECUTED) {
		printf("%s\n", output->error.message);
	}

	query_free(output);

	return ret;
}

static struct query_output* run_query(struct database *db, char *stmt)
{
	struct query_output *output;

	output = query_execute(db, stmt);
	CU_ASSERT_PTR_NOT_NULL_FATAL(output);

	// helps diagnose issues during unit tests / CI builds
	if (output->status != ST_OK_WITH_RESULTS) {
		printf("%s\n", output->error.message);
	}

	CU_ASSERT_EQUAL_FATAL(output->status, ST_OK_WITH_RESULTS);

	return output;
}

static void test_select_1(void)
{
	struct database db = {0};
	struct query_output *output;
	int64_t exp_vals[] = {123, -12345};
	int i = 0;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	CU_ASSERT_EQUAL(run_stmt(&db, "CREATE TABLE TEST (f1 INT);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (123);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO TEST VALUES (-12345);"), ST_OK_EXECUTED);

	output = run_query(&db, "SELECT * FROM TEST;");

	while (query_cur_step(&output->results) == MIDORIDB_ROW) {
		CU_ASSERT_EQUAL(query_column_int64(&output->results, 0), exp_vals[i]);
		i++;
	}

	query_free(output);
	database_close(&db);
}

static void test_select_2(void)
{
	struct database db = {0};
	struct query_output *output;
	int64_t exp_vals[4][4] = {
			{123, -12345},
			{123, -67890},
			{456, -12345},
			{456, -67890}
	};
	int i = 0;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	CU_ASSERT_EQUAL(run_stmt(&db, "CREATE TABLE A (f1 INT);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (123), (456);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "CREATE TABLE B (f2 INT);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (-12345), (-67890);"), ST_OK_EXECUTED);

	output = run_query(&db, "SELECT * FROM A, B;");

	while (query_cur_step(&output->results) == MIDORIDB_ROW) {
		CU_ASSERT_EQUAL(query_column_int64(&output->results, 0), exp_vals[i][0]);
		CU_ASSERT_EQUAL(query_column_int64(&output->results, 1), exp_vals[i][1]);
		i++;
	}

	query_free(output);
	database_close(&db);
}

static void test_select_3(void)
{
	struct database db = {0};
	struct query_output *output;
	int64_t exp_vals[2][4] = {
			{1, 1, 123, -12345},
			{3, 3, 789, -67890},
	};
	int i = 0;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	CU_ASSERT_EQUAL(run_stmt(&db, "CREATE TABLE A (id_a INT, f1 INT);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (1, 123), (2, 456), (3, 789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "CREATE TABLE B (id_b INT, f2 INT);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (1, -12345), (3, -67890);"), ST_OK_EXECUTED);

	output = run_query(&db, "SELECT * FROM A INNER JOIN B ON A.id_a = B.id_b;");

	while (query_cur_step(&output->results) == MIDORIDB_ROW) {
		CU_ASSERT_EQUAL(query_column_int64(&output->results, 0), exp_vals[i][0]);
		CU_ASSERT_EQUAL(query_column_int64(&output->results, 1), exp_vals[i][1]);
		CU_ASSERT_EQUAL(query_column_int64(&output->results, 2), exp_vals[i][2]);
		CU_ASSERT_EQUAL(query_column_int64(&output->results, 3), exp_vals[i][3]);
		i++;
	}

	query_free(output);
	database_close(&db);
}

static void test_select_4(void)
{
	struct database db = {0};
	struct query_output *output;
	int64_t exp_vals[2][6] = {
			{1, 1, 1, 123, -12345, 333},
			{3, 3, 3, 789, -67890, 666},
	};
	int i = 0;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	CU_ASSERT_EQUAL(run_stmt(&db, "CREATE TABLE A (id_a INT, f1 INT);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO A VALUES (1, 123), (2, 456), (3, 789);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "CREATE TABLE B (id_b INT, f2 INT);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO B VALUES (1, -12345), (2, -11111), (3, -67890);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "CREATE TABLE C (id_c INT, f3 INT);"), ST_OK_EXECUTED);
	CU_ASSERT_EQUAL(run_stmt(&db, "INSERT INTO C VALUES (1, 333), (3, 666), (4, 999);"), ST_OK_EXECUTED);

	output = run_query(&db, "SELECT * FROM A INNER JOIN B ON A.id_a = B.id_b INNER JOIN C ON A.id_a = C.id_c;");

	while (query_cur_step(&output->results) == MIDORIDB_ROW) {
//		printf("i: %d\n", i);
		CU_ASSERT_EQUAL(query_column_int64(&output->results, 0), exp_vals[i][0]);
		CU_ASSERT_EQUAL(query_column_int64(&output->results, 1), exp_vals[i][1]);
		CU_ASSERT_EQUAL(query_column_int64(&output->results, 2), exp_vals[i][2]);
		CU_ASSERT_EQUAL(query_column_int64(&output->results, 3), exp_vals[i][3]);
		CU_ASSERT_EQUAL(query_column_int64(&output->results, 4), exp_vals[i][4]);
		CU_ASSERT_EQUAL(query_column_int64(&output->results, 5), exp_vals[i][5]);
		i++;
	}

	query_free(output);
	database_close(&db);
}

void test_executor_select(void)
{
	/* single field */
	test_select_1();

	/* implicit join => CROSS JOIN functionally-wise */
	test_select_2();

	/* single join - inner */
	test_select_3();

	/* multiple join - inner */
	test_select_4();
}
