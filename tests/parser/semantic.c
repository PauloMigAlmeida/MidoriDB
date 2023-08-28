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
	bool ret;

	memzero(err_msg, sizeof(err_msg));

	node = build_ast(stmt);
	CU_ASSERT_PTR_NOT_NULL_FATAL(node);

	ret = semantic_analyse(db, node, err_msg, sizeof(err_msg));

	if (!ret) {
		printf("err_msg: %s\n", err_msg);
	}

	CU_ASSERT_FATAL(ret == !expect_to_fail);

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

	/* valid case - insert - no col_names; single row; recursive math insert_expr */
	prep_helper(&db, "CREATE TABLE C_1 (f1 INT, f2 INT);");
	helper(&db, "INSERT INTO C_1 VALUES ((2 + 2) * 3, 4 * (3 + 1));", false); // all integer
	prep_helper(&db, "CREATE TABLE C_2 (f1 DOUBLE, f2 DOUBLE);");
	helper(&db, "INSERT INTO C_2 VALUES ((2.0 + 2.0) * 3.0, 4.0 * (3.0 + 1.0));", false); // all double

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
	helper(&db, "INSERT INTO J (f1) VALUES ('hello');", false);
	helper(&db, "INSERT INTO J (f1) VALUES ('hellohellohello');", true); //testing varchar boundaries

	/* invalid case - insert - invalid value for column - DATE */
	prep_helper(&db, "CREATE TABLE K (f1 DATE);");
	helper(&db, "INSERT INTO K (f1) VALUES (1688253642);", true); // we don't support implicit conversion
	helper(&db, "INSERT INTO K (f1) VALUES ('2023-07-02');", false);
	helper(&db, "INSERT INTO K (f1) VALUES ('2023-07-02'), (1688253642);", true); // all values must be valid for column

	/* invalid case - insert - invalid value for column - DATETIME */
	prep_helper(&db, "CREATE TABLE L (f1 DATETIME);");
	helper(&db, "INSERT INTO L (f1) VALUES (1688253642);", true); // we don't support implicit conversion
	helper(&db, "INSERT INTO L (f1) VALUES ('2023-07-02 11:22:00');", false);

	/* invalid case - insert - messing with column opt list order */
	prep_helper(&db, "CREATE TABLE M (f1 INT, f2 DOUBLE, f3 VARCHAR(10), f4 DATE);");
	helper(&db, "INSERT INTO M (f2, f1, f4, f3) VALUES (456, 789.0, 'hello', '2023-07-02');", true);
	helper(&db, "INSERT INTO M (f2, f1, f4, f3) VALUES (789.0, 456, '2023-07-02', 'hello');", false);

	/* invalid case - insert - number of terms different from number of columns in opt_column_list */
	prep_helper(&db, "CREATE TABLE N (f1 INT, f2 DOUBLE, f3 TINYINT);");
	helper(&db, "INSERT INTO N (f2, f1, f3) VALUES (789.0);", true);
	helper(&db, "INSERT INTO N (f2, f1) VALUES (789.0, 456, TRUE);", true);
	helper(&db, "INSERT INTO N (f2, f1) VALUES (456, 789.0);", true); // wrong data type (purposely)
	helper(&db, "INSERT INTO N (f2, f1) VALUES (789.0, 456);", false);

	/* invalid case - insert - number of terms different from number of table columns*/
	prep_helper(&db, "CREATE TABLE O (f1 INT, f2 DOUBLE);");
	helper(&db, "INSERT INTO O VALUES (456);", true);
	helper(&db, "INSERT INTO O VALUES (123, 789.0);", false);

	/* invalid case - insert - leaving out column with NOT NULL constraint using opt_column_list */
	prep_helper(&db, "CREATE TABLE P (f1 INT, f2 DOUBLE NOT NULL);");
	helper(&db, "INSERT INTO P (f1) VALUES (456);", true);
	helper(&db, "INSERT INTO P (f1, f2) VALUES (123, 789.0);", false);

	/* invalid case - insert - explicitly inserting NULL into a NOT NULL column */
	prep_helper(&db, "CREATE TABLE Q (f1 INT, f2 DOUBLE NOT NULL);");
	helper(&db, "INSERT INTO Q (f1, f2) VALUES (123, NULL);", true);
	helper(&db, "INSERT INTO Q (f1, f2) VALUES (NULL, 789.0);", false);
	helper(&db, "INSERT INTO Q (f1, f2) VALUES (123, NULL), (NULL, 789.0);", true);

	/* invalid case - insert - invalid insert_expr*/
	prep_helper(&db, "CREATE TABLE U_1 (f1 INT);");
	helper(&db, "INSERT INTO U_1 VALUES (2.0 * 3.0);", true); // diff types than column supports
	helper(&db, "INSERT INTO U_1 VALUES ((2 + 2) * 3.0);", true); // mix types
	helper(&db, "INSERT INTO U_1 VALUES ((2 + 2) * 'a');", true); // string are not allowed, this isn't Python :-)
	helper(&db, "INSERT INTO U_1 VALUES ((2 + 2) / 0);", false); // mathematically invalid, but semantically valid (and would be translated to NULL)
	helper(&db, "INSERT INTO U_1 VALUES ((2 + 2) / 0.0);", true); // division by zero with mix types
	helper(&db, "INSERT INTO U_1 VALUES (-(2 + 2));", false); // negation
	prep_helper(&db, "CREATE TABLE U_2 (f1 DOUBLE);");
	helper(&db, "INSERT INTO U_2 VALUES (2 * 3);", true); // diff types than column supports
	helper(&db, "INSERT INTO U_2 VALUES ((2.0 + 2.0) * 3);", true); // mix types
	helper(&db, "INSERT INTO U_2 VALUES ((2.0 + 2.0) * 'a');", true); // string are not allowed, this isn't Python :-)
	helper(&db, "INSERT INTO U_2 VALUES ((2.0 + 2.0) / 0.0);", false); // mathematically invalid, but semantically valid (and would be translated to NULL)
	helper(&db, "INSERT INTO U_2 VALUES ((2.0 + 2.0) / 0);", true); // division by zero with mix types
	helper(&db, "INSERT INTO U_2 VALUES (-(2.0 + 2.0));", false); // negation

	database_close(&db);
}

static void delete_tests(void)
{
	struct database db = {0};

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	/* valid case - delete all */
	prep_helper(&db, "CREATE TABLE V_A (f1 INT, f2 VARCHAR(4));");
	helper(&db, "DELETE FROM V_A;", false);

	/* valid case - single condition */
	prep_helper(&db, "CREATE TABLE V_B (f1 INT, f2 VARCHAR(4));");
	helper(&db, "DELETE FROM V_B WHERE f1 = 1;", false);

	/* valid case - multiple conditions */
	prep_helper(&db, "CREATE TABLE V_C (f1 INT, f2 VARCHAR(4), f3 double);");
	helper(&db, "DELETE FROM V_C WHERE (f1 = 1 OR f2 = 'paulo') AND f3 = 42.0;", false);

	/* valid case - single condition; IN-clause */
	prep_helper(&db, "CREATE TABLE V_D (f1 INT);");
	helper(&db, "DELETE FROM V_D WHERE f1 IN (1,2,3,4);", false);

	/* valid case - multiple conditions; IN-clause  */
	prep_helper(&db, "CREATE TABLE V_E (f1 INT, f2 VARCHAR(4), f3 double, f4 int);");
	helper(&db, "DELETE FROM V_E WHERE (f1 = 1 OR f2 = 'paulo') AND (f3 = 42.0 OR f4 in (10,12));", false);

	/* invalid case - delete all */
	prep_helper(&db, "CREATE TABLE I_A (f1 INT);");
	helper(&db, "DELETE FROM I_AHKSDJ;", true);

	/* invalid case - single condition; invalid column */
	prep_helper(&db, "CREATE TABLE I_B (f1 INT);");
	helper(&db, "DELETE FROM I_B WHERE f2 = 1;", true);

	/* invalid case - multiple conditions; invalid column */
	prep_helper(&db, "CREATE TABLE I_C (f1 INT, f2 VARCHAR(4), f3 double);");
	helper(&db, "DELETE FROM I_C WHERE (f1 = 1 OR f4 = 'paulo') AND f3 = 42.0 OR 1=1;", true);

	/* invalid case - single condition; IN-clause */
	prep_helper(&db, "CREATE TABLE I_D (f1 INT);");
	helper(&db, "DELETE FROM I_D WHERE f1 IN (1,2,3,f1);", true);

	/* invalid case - multiple conditions; IN-clause  */
	prep_helper(&db, "CREATE TABLE I_E (f1 INT, f2 VARCHAR(4), f3 double, f4 int);");
	helper(&db, "DELETE FROM I_E WHERE (f1 = 1 OR f2 = 'paulo') AND (f3 = 42.0 OR f4 in (10,f1));", true);

	database_close(&db);
}

void test_semantic_analyze(void)
{
	create_tests();
	insert_tests();
	delete_tests();
}
