/*
 * semantic.c
 *
 *  Created on: 7/06/2023
 *      Author: paulo
 */

#include <tests/parser.h>
#include <parser/semantic.h>
#include <engine/query.h>

void helper(struct database *db, char *stmt, bool expect_to_fail)
{
	struct ast_node *node;
	char err_msg[1024];

	memzero(err_msg, sizeof(err_msg));

	node = build_ast(stmt);
	CU_ASSERT_PTR_NOT_NULL_FATAL(node);

	CU_ASSERT_FATAL(semantic_analyse(db, node, err_msg, sizeof(err_msg)) == !expect_to_fail);

	if (expect_to_fail) {
		printf("err_msg: %s\n", err_msg);
	}

	ast_free(node);
}

/**
 * Some statements require tables to exist before the semantic analysis take place.
 * So this a helper that should run before "helper" is invoked in some tests
 */
void prep_helper(struct database *db, char *stmt)
{
	struct query_output *output;

	output = query_execute(db, stmt);
	CU_ASSERT_PTR_NOT_NULL_FATAL(output);
	CU_ASSERT_EQUAL_FATAL(output->status, ST_OK_EXECUTED)

	free(output);
}

static void create_tests(void)
{
	struct database db = {0};
	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	/* valid case */
	helper(&db, "CREATE TABLE IF NOT EXISTS A ("
		"	f1 INTEGER PRIMARY KEY AUTO_INCREMENT,"
		"	f2 INT UNIQUE,"
		"	f3 DOUBLE NOT NULL);",
		false);

	/* valid case - pk after column definition*/
	helper(&db, "CREATE TABLE IF NOT EXISTS B ("
		"	f1 INTEGER AUTO_INCREMENT,"
		"	f2 INT UNIQUE,"
		"	PRIMARY KEY(f1));",
		false);

	/* valid case - idx after column definition*/
	helper(&db, "CREATE TABLE IF NOT EXISTS C ("
		"	f1 INTEGER AUTO_INCREMENT,"
		"	f2 INT UNIQUE,"
		"	INDEX(f1));",
		false);

	/* invalid case - invalid table name*/
	helper(&db, "CREATE TABLE iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii (f1 INT);",
		true);

	/* invalid case - invalid column name*/
	helper(&db, "CREATE TABLE A123 (iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii INT);",
		true);

	/* invalid case - column name duplicate */
	helper(&db, "CREATE TABLE IF NOT EXISTS D ("
		"	f1 INTEGER AUTO_INCREMENT,"
		"	f1 INT UNIQUE,"
		"	INDEX(f1));",
		true);

	/* invalid case - PK after column definition points to invalid column name */
	helper(&db, "CREATE TABLE IF NOT EXISTS E ("
		"	f1 INTEGER AUTO_INCREMENT,"
		"	PRIMARY KEY(f2));",
		true);

	/* invalid case - index after column definition points to invalid column name */
	helper(&db, "CREATE TABLE IF NOT EXISTS F ("
		"	f1 INTEGER AUTO_INCREMENT,"
		"	INDEX(f2));",
		true);

	database_close(&db);
}

static void insert_tests(void)
{
	struct database db = {0};

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	/* valid case - insert - no col_names; single row */
	prep_helper(&db, "CREATE TABLE A (f1 INT, f2 VARCHAR(4));");
	helper(&db, "INSERT INTO A VALUES (123, '456');", false);

	/* valid case - insert - with col_names; multiple rows */
	prep_helper(&db, "CREATE TABLE B (f1 INT, f2 VARCHAR(4));");
	helper(&db, "INSERT INTO B (f1, f2) VALUES (123, '456'), (678, '901');", false);

	/* valid case - insert - no col_names; single row; recursive math expr */
	prep_helper(&db, "CREATE TABLE C (f1 INT, f2 INT);");
	helper(&db, "INSERT INTO C VALUES ((2 + 2) * 3, 4 * (3 + 1));", false);

	/* invalid case - insert - with col_names; multiple rows; different num of terms */
	prep_helper(&db, "CREATE TABLE D (f1 INT, f2 VARCHAR(4));");
	helper(&db, "INSERT INTO D (f1, f2) VALUES (123, '456'), (678);", true);

	/* invalid case - insert - with col_names; single row; different num of terms */
	prep_helper(&db, "CREATE TABLE E (f1 INT);");
	helper(&db, "INSERT INTO E (f1) VALUES (123, '456');", true);

	/* invalid case - insert - invalid table name */
	helper(&db, "INSERT INTO iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii (f1) VALUES (123);",
		true);

	/* invalid case - insert - invalid column name */
	prep_helper(&db, "CREATE TABLE F (f1 INT);");
	helper(&db, "INSERT INTO F (iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii) VALUES (123);",
		true);

	/* invalid case - insert - invalid value for column - int */
	prep_helper(&db, "CREATE TABLE G (f1 INT);");
	helper(&db, "INSERT INTO G (f1) VALUES ('456');", true);
	helper(&db, "INSERT INTO G (f1) VALUES (456);", false);

	/* invalid case - insert - invalid value for column - double */
	prep_helper(&db, "CREATE TABLE H (f1 DOUBLE);");
	helper(&db, "INSERT INTO H (f1) VALUES (123);", true); // we don't support implicit conversion
	helper(&db, "INSERT INTO H (f1) VALUES (123.0);", false);

	/* invalid case - insert - invalid value for column - bool */
	prep_helper(&db, "CREATE TABLE I (f1 TINYINT);");
	helper(&db, "INSERT INTO I (f1) VALUES (1);", true); // we don't support implicit conversion
	helper(&db, "INSERT INTO I (f1) VALUES (TRUE);", false);

	/* invalid case - insert - invalid value for column - VARCHAR */
	prep_helper(&db, "CREATE TABLE J (f1 VARCHAR(10));");
	helper(&db, "INSERT INTO J (f1) VALUES (1);", true);
	//TODO maybe we can check boundaries for VARCHAR values in semantic values
	helper(&db, "INSERT INTO J (f1) VALUES ('hello');", false);

	/* invalid case - insert - invalid value for column - DATE */
	prep_helper(&db, "CREATE TABLE K (f1 DATE);");
	helper(&db, "INSERT INTO K (f1) VALUES (1688253642);", true); // we don't support implicit conversion
	helper(&db, "INSERT INTO K (f1) VALUES ('2023-07-02');", false);

	/* invalid case - insert - invalid value for column - DATETIME */
	prep_helper(&db, "CREATE TABLE L (f1 DATETIME);");
	helper(&db, "INSERT INTO L (f1) VALUES (1688253642);", true); // we don't support implicit conversion
	helper(&db, "INSERT INTO L (f1) VALUES ('2023-07-02 11:22:00');", false);

	/* invalid case - insert - messing with column opt list order */
	prep_helper(&db, "CREATE TABLE M (f1 INT, f2 DOUBLE, f3 VARCHAR(10), f4 DATE);");
	helper(&db, "INSERT INTO M (f2, f1, f4, f3) VALUES (456, 789.0, 'hello', '2023-07-02');", true);
	helper(&db, "INSERT INTO M (f2, f1, f4, f3) VALUES (789.0, 456, '2023-07-02', 'hello');", false);

	database_close(&db);
}

void test_semantic_analyze(void)
{
	create_tests();
	insert_tests();
}
