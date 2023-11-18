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

void test_executor_select(void)
{
	/* single field */
	test_select_1();
}
