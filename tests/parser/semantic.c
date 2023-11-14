/*
 * semantic.c
 *
 *  Created on: 7/06/2023
 *      Author: paulo
 */

#include <tests/parser.h>
#include <parser/semantic.h>
#include <engine/query.h>

static void helper(struct database *db, char *stmt, bool expect_to_fail)
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
static void prep_helper(struct database *db, char *stmt)
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
	helper(&db, "DELETE FROM V_D WHERE f1 IN (1,2,3,NULL);", false);

	/* valid case - multiple conditions; IN-clause  */
	prep_helper(&db, "CREATE TABLE V_E (f1 INT, f2 VARCHAR(4), f3 double, f4 int);");
	helper(&db, "DELETE FROM V_E WHERE (f1 = 1 OR f2 = 'paulo') AND (f3 = 42.0 OR f4 in (10,12));", false);

	/* valid case - single condition; NULL comparison */
	prep_helper(&db, "CREATE TABLE V_F (f1 INT);");
	helper(&db, "DELETE FROM V_F WHERE f1 IS NULL;", false);
	helper(&db, "DELETE FROM V_F WHERE f1 IS NOT NULL;", false);
	helper(&db, "DELETE FROM V_F WHERE f1 = NULL;", false);
	helper(&db, "DELETE FROM V_F WHERE f1 <> NULL;", false);
	helper(&db, "DELETE FROM V_F WHERE f1 != NULL;", false);
	helper(&db, "DELETE FROM V_F WHERE NULL != NULL;", false);
	helper(&db, "DELETE FROM V_F WHERE NULL <> NULL;", false);
	helper(&db, "DELETE FROM V_F WHERE NULL = NULL;", false);

	/* valid case - multiple conditions; valid field->value types configuration  */
	prep_helper(&db, "CREATE TABLE V_G (f1 INT, f2 VARCHAR(4), f3 double, f4 int, f5 DATE);");
	helper(&db, "DELETE FROM V_G WHERE (f1 = 1 OR f2 = 'paulo') AND (f3 = 42.0 OR f4 in (10,12));", false);
	helper(&db, "DELETE FROM V_G WHERE f2 = 'paulo' AND f4 in (10,12);", false);
	helper(&db, "DELETE FROM V_G WHERE (f1 = NULL OR f2 IS NOT NULL) AND (f3 IS NULL OR f4 NOT in (NULL,10));",
	false);
	helper(&db, "DELETE FROM V_G WHERE f5 > '2022-02-02';", false);

	/* valid case - single condition; VARCHAR comparison */
	prep_helper(&db, "CREATE TABLE V_H (f1 VARCHAR(10));");
	helper(&db, "DELETE FROM V_H WHERE f1 = 'paulo';", false);
	helper(&db, "DELETE FROM V_H WHERE f1 <> 'paulo';", false);
	helper(&db, "DELETE FROM V_H WHERE f1 != 'paulo';", false);

	/* valid case - single condition; [field|value] cmp [field|value] */
	prep_helper(&db, "CREATE TABLE V_I (f1 int, f2 double, f3 VARCHAR(10));");
	// int
	helper(&db, "DELETE FROM V_I WHERE f1 = 1;", false); // field-to-value
	helper(&db, "DELETE FROM V_I WHERE f1 > 1;", false);
	helper(&db, "DELETE FROM V_I WHERE f1 >= 1;", false);
	helper(&db, "DELETE FROM V_I WHERE f1 < 1;", false);
	helper(&db, "DELETE FROM V_I WHERE f1 <= 1;", false);
	helper(&db, "DELETE FROM V_I WHERE f1 <> 1;", false);
	helper(&db, "DELETE FROM V_I WHERE 1 = f1;", false); // value-to-field
	helper(&db, "DELETE FROM V_I WHERE 1 > f1;", false);
	helper(&db, "DELETE FROM V_I WHERE 1 >= f1;", false);
	helper(&db, "DELETE FROM V_I WHERE 1 < f1;", false);
	helper(&db, "DELETE FROM V_I WHERE 1 <= f1;", false);
	helper(&db, "DELETE FROM V_I WHERE 1 <> f1;", false);
	helper(&db, "DELETE FROM V_I WHERE f1 = f1;", false); // field-to-field
	helper(&db, "DELETE FROM V_I WHERE f1 > f1;", false);
	helper(&db, "DELETE FROM V_I WHERE f1 >= f1;", false);
	helper(&db, "DELETE FROM V_I WHERE f1 < f1;", false);
	helper(&db, "DELETE FROM V_I WHERE f1 <= f1;", false);
	helper(&db, "DELETE FROM V_I WHERE f1 <> f1;", false);
	helper(&db, "DELETE FROM V_I WHERE 1 = 1;", false); // value-to-value
	helper(&db, "DELETE FROM V_I WHERE 1 > 1;", false);
	helper(&db, "DELETE FROM V_I WHERE 1 >= 1;", false);
	helper(&db, "DELETE FROM V_I WHERE 1 < 1;", false);
	helper(&db, "DELETE FROM V_I WHERE 1 <= 1;", false);
	helper(&db, "DELETE FROM V_I WHERE 1 <> 1;", false);
	// double
	helper(&db, "DELETE FROM V_I WHERE f2 = 1.0;", false); // field-to-value
	helper(&db, "DELETE FROM V_I WHERE f2 > 1.0;", false);
	helper(&db, "DELETE FROM V_I WHERE f2 >= 1.0;", false);
	helper(&db, "DELETE FROM V_I WHERE f2 < 1.0;", false);
	helper(&db, "DELETE FROM V_I WHERE f2 <= 1.0;", false);
	helper(&db, "DELETE FROM V_I WHERE f2 <> 1.0;", false);
	helper(&db, "DELETE FROM V_I WHERE 1.0 = f2;", false); // value-to-field
	helper(&db, "DELETE FROM V_I WHERE 1.0 > f2;", false);
	helper(&db, "DELETE FROM V_I WHERE 1.0 >= f2;", false);
	helper(&db, "DELETE FROM V_I WHERE 1.0 < f2;", false);
	helper(&db, "DELETE FROM V_I WHERE 1.0 <= f2;", false);
	helper(&db, "DELETE FROM V_I WHERE 1.0 <> f2;", false);
	helper(&db, "DELETE FROM V_I WHERE f2 = f2;", false); // field-to-field
	helper(&db, "DELETE FROM V_I WHERE f2 > f2;", false);
	helper(&db, "DELETE FROM V_I WHERE f2 >= f2;", false);
	helper(&db, "DELETE FROM V_I WHERE f2 < f2;", false);
	helper(&db, "DELETE FROM V_I WHERE f2 <= f2;", false);
	helper(&db, "DELETE FROM V_I WHERE f2 <> f2;", false);
	helper(&db, "DELETE FROM V_I WHERE 1.0 = 1.0;", false); // value-to-value
	helper(&db, "DELETE FROM V_I WHERE 1.0 > 1.0;", false);
	helper(&db, "DELETE FROM V_I WHERE 1.0 >= 1.0;", false);
	helper(&db, "DELETE FROM V_I WHERE 1.0 < 1.0;", false);
	helper(&db, "DELETE FROM V_I WHERE 1.0 <= 1.0;", false);
	helper(&db, "DELETE FROM V_I WHERE 1.0 <> 1.0;", false);
	// string
	helper(&db, "DELETE FROM V_I WHERE f3 = 'test';", false); // field-to-value
	helper(&db, "DELETE FROM V_I WHERE f3 <> 'test';", false);
	helper(&db, "DELETE FROM V_I WHERE 'test' = f3;", false); // value-to-field
	helper(&db, "DELETE FROM V_I WHERE 'test' <> f3;", false);
	helper(&db, "DELETE FROM V_I WHERE f3 = f3;", false); // field-to-field
	helper(&db, "DELETE FROM V_I WHERE f3 <> f3;", false);
	helper(&db, "DELETE FROM V_I WHERE 'test' = 'test';", false); // value-to-value
	helper(&db, "DELETE FROM V_I WHERE 'test' <> 'test';", false);
	// null
	helper(&db, "DELETE FROM V_I WHERE f1 = NULL;", false); // field-to-value
	helper(&db, "DELETE FROM V_I WHERE f1 <> NULL;", false);
	helper(&db, "DELETE FROM V_I WHERE NULL = f1;", false); // value-to-field
	helper(&db, "DELETE FROM V_I WHERE NULL <> f1;", false);
	helper(&db, "DELETE FROM V_I WHERE NULL = NULL;", false); // value-to-value
	helper(&db, "DELETE FROM V_I WHERE NULL <> NULL;", false);

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

	/* invalid case - single condition; NULL comparison */
	prep_helper(&db, "CREATE TABLE I_F (f1 INT);");
	helper(&db, "DELETE FROM I_F WHERE f1 > NULL;", true);
	helper(&db, "DELETE FROM I_F WHERE f1 < NULL;", true);
	helper(&db, "DELETE FROM I_F WHERE f1 >= NULL;", true);
	helper(&db, "DELETE FROM I_F WHERE f1 <= NULL;", true);
	helper(&db, "DELETE FROM I_F WHERE NULL IS NULL;", true);
	helper(&db, "DELETE FROM I_F WHERE NULL IS NOT NULL;", true);

	/* invalid case - multiple conditions; wrong field->value types configuration  */
	prep_helper(&db, "CREATE TABLE I_G (f1 INT, f2 VARCHAR(4), f3 double, f4 int, f5 DATE);");
	helper(&db, "DELETE FROM I_G WHERE f1 = 'paulo';", true);
	helper(&db, "DELETE FROM I_G WHERE f1 = 1 AND f3 > 'paulo';", true);
	helper(&db, "DELETE FROM I_G WHERE f1 = 1 AND f2 > 'paulo' AND f3 in (12, 42.0);", true);
	helper(&db, "DELETE FROM I_G WHERE (f1 = 1 OR f2 = 'paulo') AND (f3 = 42.0 OR f4 in (10.0,12));", true);
	helper(&db, "DELETE FROM I_G WHERE f5 > '2022-002-02';", true);

	/* invalid case - single condition; VARCHAR comparison */
	prep_helper(&db, "CREATE TABLE I_H (f1 VARCHAR(10));");
	helper(&db, "DELETE FROM I_H WHERE f1 >= 'paulo';", true);
	helper(&db, "DELETE FROM I_H WHERE f1 <= 'paulo';", true);

	/* invalid case - single condition; [field|value] cmp [field|value] */
	prep_helper(&db, "CREATE TABLE I_I (f1 int, f3 VARCHAR(10));");
	// string
	helper(&db, "DELETE FROM I_I WHERE f3 > 'test';", true); // field-to-value
	helper(&db, "DELETE FROM I_I WHERE f3 >= 'test';", true);
	helper(&db, "DELETE FROM I_I WHERE f3 < 'test';", true);
	helper(&db, "DELETE FROM I_I WHERE f3 <= 'test';", true);
	helper(&db, "DELETE FROM I_I WHERE 'test' > f3;", true); // value-to-field
	helper(&db, "DELETE FROM I_I WHERE 'test' >= f3;", true);
	helper(&db, "DELETE FROM I_I WHERE 'test' < f3;", true);
	helper(&db, "DELETE FROM I_I WHERE 'test' <= f3;", true);
	helper(&db, "DELETE FROM I_I WHERE f3 > f3;", true); // field-to-field
	helper(&db, "DELETE FROM I_I WHERE f3 >= f3;", true);
	helper(&db, "DELETE FROM I_I WHERE f3 < f3;", true);
	helper(&db, "DELETE FROM I_I WHERE f3 <= f3;", true);
	helper(&db, "DELETE FROM I_I WHERE 'test' > 'test';", true); // value-to-value
	helper(&db, "DELETE FROM I_I WHERE 'test' >= 'test';", true);
	helper(&db, "DELETE FROM I_I WHERE 'test' < 'test';", true);
	helper(&db, "DELETE FROM I_I WHERE 'test' <= 'test';", true);
	// null
	helper(&db, "DELETE FROM I_I WHERE f1 > NULL;", true); // field-to-value
	helper(&db, "DELETE FROM I_I WHERE f1 >= NULL;", true);
	helper(&db, "DELETE FROM I_I WHERE f1 < NULL;", true);
	helper(&db, "DELETE FROM I_I WHERE f1 <= NULL;", true);
	helper(&db, "DELETE FROM I_I WHERE NULL > f1;", true); // value-to-field
	helper(&db, "DELETE FROM I_I WHERE NULL >= f1;", true);
	helper(&db, "DELETE FROM I_I WHERE NULL < f1;", true);
	helper(&db, "DELETE FROM I_I WHERE NULL <= f1;", true);
	helper(&db, "DELETE FROM I_I WHERE NULL > NULL;", true); // value-to-value
	helper(&db, "DELETE FROM I_I WHERE NULL >= NULL;", true);
	helper(&db, "DELETE FROM I_I WHERE NULL < NULL;", true);
	helper(&db, "DELETE FROM I_I WHERE NULL <= NULL;", true);

	database_close(&db);
}

static void update_tests(void)
{
	struct database db = {0};

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	/* valid case - update all */
	prep_helper(&db, "CREATE TABLE V_A (f1 INT, f2 VARCHAR(4));");
	helper(&db, "UPDATE V_A SET f1 = 42;", false);

	/* valid case - single condition */
	prep_helper(&db, "CREATE TABLE V_B (f1 INT, f2 VARCHAR(4));");
	helper(&db, "UPDATE V_B SET f1 = 42 WHERE f1 = 1;", false);

	/* valid case - multiple conditions */
	prep_helper(&db, "CREATE TABLE V_C (f1 INT, f2 VARCHAR(4), f3 double);");
	helper(&db, "UPDATE V_C SET f1 = 42 WHERE (f1 = 1 OR f2 = 'paulo') AND f3 = 42.0;", false);

	/* valid case - single condition; IN-clause */
	prep_helper(&db, "CREATE TABLE V_D (f1 INT);");
	helper(&db, "UPDATE V_D SET f1 = 42 WHERE f1 IN (1,2,3,NULL);", false);

	/* valid case - multiple conditions; IN-clause  */
	prep_helper(&db, "CREATE TABLE V_E (f1 INT, f2 VARCHAR(4), f3 double, f4 int);");
	helper(&db, "UPDATE V_E SET f1 = 42 WHERE (f1 = 1 OR f2 = 'paulo') AND (f3 = 42.0 OR f4 in (10,12));", false);

	/* valid case - single condition; NULL comparison */
	prep_helper(&db, "CREATE TABLE V_F (f1 INT);");
	helper(&db, "UPDATE V_F SET f1 = 42 WHERE f1 IS NULL;", false);
	helper(&db, "UPDATE V_F SET f1 = 42 WHERE f1 IS NOT NULL;", false);
	helper(&db, "UPDATE V_F SET f1 = 42 WHERE f1 = NULL;", false);
	helper(&db, "UPDATE V_F SET f1 = 42 WHERE f1 <> NULL;", false);
	helper(&db, "UPDATE V_F SET f1 = 42 WHERE f1 != NULL;", false);
	helper(&db, "UPDATE V_F SET f1 = 42 WHERE NULL != NULL;", false);
	helper(&db, "UPDATE V_F SET f1 = 42 WHERE NULL <> NULL;", false);
	helper(&db, "UPDATE V_F SET f1 = 42 WHERE NULL = NULL;", false);

	/* valid case - multiple conditions; valid field->value types configuration  */
	prep_helper(&db, "CREATE TABLE V_G (f1 INT, f2 VARCHAR(4), f3 double, f4 int, f5 DATE);");
	helper(&db, "UPDATE V_G SET f1 = 42 WHERE (f1 = 1 OR f2 = 'paulo') AND (f3 = 42.0 OR f4 in (10,12));", false);
	helper(&db, "UPDATE V_G SET f1 = 42 WHERE f2 = 'paulo' AND f4 in (10,12);", false);
	helper(&db,
		"UPDATE V_G SET f1 = 42 WHERE (f1 = NULL OR f2 IS NOT NULL) AND (f3 IS NULL OR f4 NOT in (NULL,10));",
		false);
	helper(&db, "UPDATE V_G SET f1 = 42 WHERE f5 > '2022-02-02';", false);

	/* valid case - single condition; VARCHAR comparison */
	prep_helper(&db, "CREATE TABLE V_H (f1 VARCHAR(10));");
	helper(&db, "UPDATE V_H SET f1 = '42' WHERE f1 = 'paulo';", false);
	helper(&db, "UPDATE V_H SET f1 = '42' WHERE f1 <> 'paulo';", false);
	helper(&db, "UPDATE V_H SET f1 = '42' WHERE f1 != 'paulo';", false);

	/* valid case - single condition; [field|value] cmp [field|value] */
	prep_helper(&db, "CREATE TABLE V_I (f1 int, f2 double, f3 VARCHAR(10));");
	// int
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f1 = 1;", false); // field-to-value
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f1 > 1;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f1 >= 1;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f1 < 1;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f1 <= 1;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f1 <> 1;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1 = f1;", false); // value-to-field
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1 > f1;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1 >= f1;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1 < f1;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1 <= f1;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1 <> f1;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f1 = f1;", false); // field-to-field
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f1 > f1;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f1 >= f1;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f1 < f1;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f1 <= f1;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f1 <> f1;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1 = 1;", false); // value-to-value
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1 > 1;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1 >= 1;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1 < 1;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1 <= 1;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1 <> 1;", false);
	// double
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f2 = 1.0;", false); // field-to-value
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f2 > 1.0;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f2 >= 1.0;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f2 < 1.0;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f2 <= 1.0;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f2 <> 1.0;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1.0 = f2;", false); // value-to-field
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1.0 > f2;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1.0 >= f2;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1.0 < f2;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1.0 <= f2;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1.0 <> f2;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f2 = f2;", false); // field-to-field
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f2 > f2;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f2 >= f2;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f2 < f2;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f2 <= f2;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f2 <> f2;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1.0 = 1.0;", false); // value-to-value
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1.0 > 1.0;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1.0 >= 1.0;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1.0 < 1.0;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1.0 <= 1.0;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 1.0 <> 1.0;", false);
	// string
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f3 = 'test';", false); // field-to-value
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f3 <> 'test';", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 'test' = f3;", false); // value-to-field
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 'test' <> f3;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f3 = f3;", false); // field-to-field
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f3 <> f3;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 'test' = 'test';", false); // value-to-value
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE 'test' <> 'test';", false);
	// null
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f1 = NULL;", false); // field-to-value
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE f1 <> NULL;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE NULL = f1;", false); // value-to-field
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE NULL <> f1;", false);
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE NULL = NULL;", false); // value-to-value
	helper(&db, "UPDATE V_I SET f1 = 42 WHERE NULL <> NULL;", false);

	/* invalid case - update all; invalid table*/
	prep_helper(&db, "CREATE TABLE I_A (f1 INT);");
	helper(&db, "UPDATE I_AHKSDJ SET f1=1;", true);

	/* invalid case - single condition; invalid column */
	prep_helper(&db, "CREATE TABLE I_B (f1 INT);");
	helper(&db, "UPDATE I_B SET f1 = 42 WHERE f2 = 1;", true);
	helper(&db, "UPDATE I_B SET f2 = 42 WHERE f1 = 1;", true);

	/* invalid case - multiple conditions; invalid column */
	prep_helper(&db, "CREATE TABLE I_C (f1 INT, f2 VARCHAR(4), f3 double);");
	helper(&db, "UPDATE I_C SET f1 = 42 WHERE (f1 = 1 OR f4 = 'paulo') AND f3 = 42.0 OR 1=1;", true);

	/* invalid case - single condition; IN-clause */
	prep_helper(&db, "CREATE TABLE I_D (f1 INT);");
	helper(&db, "UPDATE I_D SET f1 = 42 WHERE f1 IN (1,2,3,f1);", true);

	/* invalid case - multiple conditions; IN-clause  */
	prep_helper(&db, "CREATE TABLE I_E (f1 INT, f2 VARCHAR(4), f3 double, f4 int);");
	helper(&db, "UPDATE I_E SET f1 = 42 WHERE (f1 = 1 OR f2 = 'paulo') AND (f3 = 42.0 OR f4 in (10,f1));", true);

	/* invalid case - single condition; NULL comparison */
	prep_helper(&db, "CREATE TABLE I_F (f1 INT);");
	helper(&db, "UPDATE I_F SET f1 = 42 WHERE f1 > NULL;", true);
	helper(&db, "UPDATE I_F SET f1 = 42 WHERE f1 < NULL;", true);
	helper(&db, "UPDATE I_F SET f1 = 42 WHERE f1 >= NULL;", true);
	helper(&db, "UPDATE I_F SET f1 = 42 WHERE f1 <= NULL;", true);
	helper(&db, "UPDATE I_F SET f1 = 42 WHERE NULL IS NULL;", true);
	helper(&db, "UPDATE I_F SET f1 = 42 WHERE NULL IS NOT NULL;", true);

	/* invalid case - multiple conditions; wrong field->value types configuration  */
	prep_helper(&db, "CREATE TABLE I_G (f1 INT, f2 VARCHAR(4), f3 double, f4 int, f5 DATE);");
	helper(&db, "UPDATE I_G SET f1 = 42 WHERE f1 = 'paulo';", true);
	helper(&db, "UPDATE I_G SET f1 = 42 WHERE f1 = 1 AND f3 > 'paulo';", true);
	helper(&db, "UPDATE I_G SET f1 = 42 WHERE f1 = 1 AND f2 > 'paulo' AND f3 in (12, 42.0);", true);
	helper(&db, "UPDATE I_G SET f1 = 42 WHERE (f1 = 1 OR f2 = 'paulo') AND (f3 = 42.0 OR f4 in (10.0,12));", true);
	helper(&db, "UPDATE I_G SET f1 = 42 WHERE f5 > '2022-002-02';", true);
	helper(&db, "UPDATE I_G SET f1 = '42' WHERE f1 = 1;", true);

	/* invalid case - single condition; VARCHAR comparison */
	prep_helper(&db, "CREATE TABLE I_H (f1 VARCHAR(10));");
	helper(&db, "UPDATE I_H SET f1 = 42 WHERE f1 >= 'paulo';", true);
	helper(&db, "UPDATE I_H SET f1 = 42 WHERE f1 <= 'paulo';", true);

	/* invalid case - single condition; [field|value] cmp [field|value] */
	prep_helper(&db, "CREATE TABLE I_I (f1 int, f3 VARCHAR(10));");
	// string
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE f3 > 'test';", true); // field-to-value
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE f3 >= 'test';", true);
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE f3 < 'test';", true);
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE f3 <= 'test';", true);
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE 'test' > f3;", true); // value-to-field
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE 'test' >= f3;", true);
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE 'test' < f3;", true);
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE 'test' <= f3;", true);
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE f3 > f3;", true); // field-to-field
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE f3 >= f3;", true);
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE f3 < f3;", true);
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE f3 <= f3;", true);
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE 'test' > 'test';", true); // value-to-value
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE 'test' >= 'test';", true);
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE 'test' < 'test';", true);
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE 'test' <= 'test';", true);
	// null
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE f1 > NULL;", true); // field-to-value
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE f1 >= NULL;", true);
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE f1 < NULL;", true);
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE f1 <= NULL;", true);
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE NULL > f1;", true); // value-to-field
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE NULL >= f1;", true);
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE NULL < f1;", true);
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE NULL <= f1;", true);
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE NULL > NULL;", true); // value-to-value
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE NULL >= NULL;", true);
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE NULL < NULL;", true);
	helper(&db, "UPDATE I_I SET f1 = 42 WHERE NULL <= NULL;", true);

	database_close(&db);
}

static void select_tests(void)
{
	struct database db = {0};

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);

	/* valid case - select from table */
	prep_helper(&db, "CREATE TABLE V_A_1 (f1 INT);");
	prep_helper(&db, "CREATE TABLE V_A_2 (f2 INT);");
	prep_helper(&db, "CREATE TABLE V_A_3 (f3 INT);");
	helper(&db, "SELECT f1 FROM V_A_1;", false);
	helper(&db, "SELECT f1, f2, f3 FROM V_A_1, V_A_2, V_A_3;", false);
	helper(&db, "SELECT * FROM V_A_1 JOIN V_A_2 ON f1 = f2 JOIN V_A_3 ON f2 = f3;", false);

	/* valid case - table aliases */
	prep_helper(&db, "CREATE TABLE V_B_1 (f1 INT);");
	prep_helper(&db, "CREATE TABLE V_B_2 (f2 INT);");
	prep_helper(&db, "CREATE TABLE V_B_3 (f3 INT);");
	helper(&db, "SELECT f1 FROM V_B_1 as v;", false);
	helper(&db, "SELECT v1.f1, v2.f2, f3 FROM V_B_1 as v1, V_B_2 as v2, V_B_3;", false);
	helper(&db, "SELECT * FROM V_B_1 v1 JOIN V_B_2 v2 ON v1.f1 = v2.f2 JOIN V_B_3 ON v2.f2 = f3;", false);

	/* valid case - column aliases */
	prep_helper(&db, "CREATE TABLE V_C_1 (f1 INT);");
	prep_helper(&db, "CREATE TABLE V_C_2 (f2 INT);");
	prep_helper(&db, "CREATE TABLE V_C_3 (f3 INT);");
	helper(&db, "SELECT f1 as x FROM V_C_1;", false);
	helper(&db, "SELECT y.f1 as w FROM V_C_1 as y;", false);
	helper(&db, "SELECT v1.f1 as f4, v2.f2, f3 FROM V_C_1 as v1, V_C_2 as v2, V_C_3;", false);
	helper(&db, "SELECT f1 / 2 as val FROM V_C_1;", false);
	helper(&db, "SELECT y.f1 / 2 as val FROM V_C_1 y;", false);
	helper(&db, "SELECT count(f1) as val FROM V_C_1;", false);

	/* valid case - column name */
	prep_helper(&db, "CREATE TABLE V_D_1 (f1 INT);");
	prep_helper(&db, "CREATE TABLE V_D_2 (f2 INT);");
	prep_helper(&db, "CREATE TABLE V_D_3 (f3 INT);");
	helper(&db, "SELECT 123 as x;", false);
	helper(&db, "SELECT f1 as x FROM V_D_1;", false);
	helper(&db, "SELECT f1, f2 FROM V_D_1, V_D_2;", false);
	helper(&db, "SELECT f1, f2, f3 FROM V_D_1 JOIN V_D_2 ON f1 = f2 JOIN V_D_3 ON f2 = f3;", false);
	helper(&db, "SELECT f1 / 2 as val FROM V_D_1;", false);
	helper(&db, "SELECT x.f1 FROM V_D_1 as x WHERE x.f1 = 1;", false);
	helper(&db, "SELECT f1 / 2 as val FROM V_D_1 WHERE val > 2;", false); // where
	helper(&db, "SELECT f1 / 2 as val FROM V_D_1 WHERE f1 > 2;", false);
	helper(&db, "SELECT x.f1 / 2 as val FROM V_D_1 as x;", false);
	helper(&db, "SELECT V_D_1.f1 / 2 as val FROM V_D_1;", false);
	helper(&db, "SELECT x.f1 FROM V_D_1 as x WHERE x.f1 > 2;", false);
	helper(&db, "SELECT V_D_1.f1 / 2 as val FROM V_D_1 WHERE V_D_1.f1 > 2;", false);
	helper(&db, "SELECT x.f1 / 2 as val FROM V_D_1 as x WHERE x.f1 > 2;", false);

	/* valid case - where clause */
	prep_helper(&db, "CREATE TABLE V_E_1 (f1 INT);");
	prep_helper(&db, "CREATE TABLE V_E_2 (f2 VARCHAR(10));");
	helper(&db, "SELECT x.f1 FROM V_E_1 as x WHERE x.f1 = 1;", false);
	helper(&db, "SELECT f1 FROM V_E_1 WHERE f1 = 1;", false);
	helper(&db, "SELECT f1 FROM V_E_1 WHERE f1 = 1 AND f1 = 2 OR 3 = 4;", false);
	helper(&db, "SELECT f1 FROM V_E_1 WHERE f1 = 1 AND f1 = 2 OR (3 * (5 + f1)) = 4;", false);
	helper(&db, "SELECT f1 FROM V_E_1 WHERE f1 + 2 > 10 + 10;", false);
	helper(&db, "SELECT f2 FROM V_E_2 WHERE f2 LIKE 'MIDORIDB%';", false);

	/* valid case - group-by clause */
	prep_helper(&db, "CREATE TABLE V_F_1 (f1 INT);");
	prep_helper(&db, "CREATE TABLE V_F_2 (f2 INT, f3 INT);");
	helper(&db, "SELECT f1 / 2 as val FROM V_F_1 GROUP BY val;", false); // group by
	helper(&db, "SELECT f1 FROM V_F_1 GROUP BY f1;", false);
	helper(&db, "SELECT x.f1 FROM V_F_1 as x GROUP BY x.f1;", false);
	helper(&db, "SELECT x.f2, x.f3 FROM V_F_2 as x GROUP BY x.f2, x.f3;", false);
	helper(&db, "SELECT f2, COUNT(f3) FROM V_F_2 GROUP BY f2;", false); // Should be okay!
	helper(&db, "SELECT f2, COUNT(f3) as val FROM V_F_2 GROUP BY f2;", false);

	/* valid case - order-by clause */
	prep_helper(&db, "CREATE TABLE V_G_1 (f1 INT);");
	helper(&db, "SELECT f1 / 2 as val FROM V_G_1 ORDER BY val DESC;", false); // order by
	helper(&db, "SELECT f1 FROM V_G_1 ORDER BY f1 DESC;", false);
	helper(&db, "SELECT f1 FROM V_G_1 ORDER BY f1;", false);
	helper(&db, "SELECT V_G_1.f1 FROM V_G_1 ORDER BY V_G_1.f1 DESC;", false);
	helper(&db, "SELECT x.f1 FROM V_G_1 as x ORDER BY x.f1 DESC;", false);
	helper(&db, "SELECT x.f1 FROM V_G_1 as x ORDER BY x.f1;", false);

	/* valid case - COUNT function */
	prep_helper(&db, "CREATE TABLE V_H_1 (f1 INT);");
	helper(&db, "SELECT COUNT(*) FROM V_H_1;", false);
	helper(&db, "SELECT COUNT(f1) FROM V_H_1;", false);
	helper(&db, "SELECT COUNT(f1), COUNT(f1) as val FROM V_H_1;", false);
	helper(&db, "SELECT COUNT(V_H_1.f1) FROM V_H_1;", false);
	helper(&db, "SELECT COUNT(x.f1) FROM V_H_1 as x;", false);
	helper(&db, "SELECT COUNT(x.f1) as y FROM V_H_1 as x HAVING y > 0;", false);

	/* valid case - HAVING function */
	prep_helper(&db, "CREATE TABLE V_I_1 (f1 INT, f2 INT);");
	helper(&db, "SELECT COUNT(*) FROM V_I_1 HAVING COUNT(*) > 1;", false); // count
	helper(&db, "SELECT COUNT(f1) FROM V_I_1 HAVING COUNT(f1) > 1;", false);
	helper(&db, "SELECT COUNT(V_I_1.f1) FROM V_I_1 HAVING COUNT(V_I_1.f1) > 1;", false);
	helper(&db, "SELECT COUNT(x.f1) FROM V_I_1 as x HAVING COUNT(x.f1) > 1;", false);
	helper(&db, "SELECT COUNT(x.f1) as y FROM V_I_1 as x HAVING y > 0;", false);
	helper(&db, "SELECT f1 FROM V_I_1 GROUP BY f1 HAVING f1 > 0;", false); // group-by field
	helper(&db, "SELECT f1 FROM V_I_1 HAVING f1 > 0;", false);
	helper(&db, "SELECT f1 as x FROM V_I_1 GROUP BY x HAVING x > 0;", false); // group-by field + alias
	helper(&db, "SELECT f1 as x FROM V_I_1 HAVING x > 0;", false);
	helper(&db, "SELECT f2 FROM V_I_1 GROUP BY f2 HAVING COUNT(*) > 1;", false); // COUNT has an special treatment

	/* valid case - JOIN expressions */
	prep_helper(&db, "CREATE TABLE V_J_1 (f1 INT);");
	prep_helper(&db, "CREATE TABLE V_J_2 (f2 INT);");
	prep_helper(&db, "CREATE TABLE V_J_3 (f3 INT);");
	helper(&db, "SELECT * FROM V_J_1 JOIN V_J_2 ON f1 = f2;", false); // single join; expr (name)
	helper(&db, "SELECT * FROM V_J_1 a JOIN V_J_2 b ON a.f1 = b.f2;", false); // single join; fieldname
	helper(&db, "SELECT * FROM V_J_1 JOIN V_J_2 ON V_J_1.f1 = V_J_2.f2;", false); // single join; fqfield
	helper(&db, "SELECT * FROM V_J_1 JOIN V_J_2 ON f1 = f2 JOIN V_J_3 ON f2 = f3;", false); // multi-join; expr (name)
	helper(&db, "SELECT * FROM V_J_1 a JOIN V_J_2 b ON a.f1 = b.f2 JOIN V_J_3 c ON b.f2 = c.f3;", false); // multi-join; fieldname
	helper(&db, "SELECT * FROM V_J_1 JOIN V_J_2 ON V_J_1.f1 = V_J_2.f2 JOIN V_J_3 ON V_J_2.f2 = V_J_3.f3;", false); // multi-join; fqfield

	/* valid case - ISXIN expressions */
	prep_helper(&db, "CREATE TABLE V_K_1 (f1 INT, f2 VARCHAR(1), f3 DOUBLE);");
	helper(&db, "SELECT * FROM V_K_1 WHERE f1 IN (1,2,3);", false);
	helper(&db, "SELECT * FROM V_K_1 WHERE V_K_1.f1 IN (1, 2, 3);", false);
	helper(&db, "SELECT * FROM V_K_1 a WHERE a.f1 IN (1, 2, 3);", false);
	helper(&db, "SELECT * FROM V_K_1 WHERE f1 NOT IN (1,2,3);", false);

	helper(&db, "SELECT * FROM V_K_1 WHERE f2 IN ('1','2','3');", false);
	helper(&db, "SELECT * FROM V_K_1 WHERE V_K_1.f2 IN ('1','2','3');", false);
	helper(&db, "SELECT * FROM V_K_1 a WHERE a.f2 IN ('1','2','3');", false);
	helper(&db, "SELECT * FROM V_K_1 WHERE f2 NOT IN ('1','2','3');", false);

	helper(&db, "SELECT * FROM V_K_1 WHERE f3 IN (1.0, 2.0, 3.0);", false);
	helper(&db, "SELECT * FROM V_K_1 WHERE V_K_1.f3 IN (1.0, 2.0, 3.0);", false);
	helper(&db, "SELECT * FROM V_K_1 a WHERE a.f3 IN (1.0, 2.0, 3.0);", false);
	helper(&db, "SELECT * FROM V_K_1 WHERE f3 NOT IN (1.0, 2.0, 3.0);", false);

	/* valid case - NULL comparison */
	prep_helper(&db, "CREATE TABLE V_L_1 (f1 INT);");
	helper(&db, "SELECT * FROM V_L_1 WHERE f1 IS NULL;", false);
	helper(&db, "SELECT * FROM V_L_1 WHERE f1 IS NOT NULL;", false);
	helper(&db, "SELECT * FROM V_L_1 WHERE V_L_1.f1 IS NULL;", false);
	helper(&db, "SELECT * FROM V_L_1 a WHERE a.f1 IS NULL;", false);
	helper(&db, "SELECT * FROM V_L_1 WHERE f1 = NULL;", false);
	helper(&db, "SELECT * FROM V_L_1 WHERE f1 <> NULL;", false);
	helper(&db, "SELECT * FROM V_L_1 WHERE f1 != NULL;", false);
	helper(&db, "SELECT * FROM V_L_1 WHERE NULL != NULL;", false);
	helper(&db, "SELECT * FROM V_L_1 WHERE NULL <> NULL;", false);
	helper(&db, "SELECT * FROM V_L_1 WHERE NULL = NULL;", false);

	/* valid case - single condition; [field|value] cmp [field|value] */
	prep_helper(&db, "CREATE TABLE V_M_1 (f1 INT, f2 DOUBLE, f3 VARCHAR(10));");
	// int
	helper(&db, "SELECT * FROM V_M_1 WHERE f1 = 1;", false); // field-to-value
	helper(&db, "SELECT * FROM V_M_1 WHERE f1 > 1;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE f1 >= 1;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE f1 < 1;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE f1 <= 1;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE f1 <> 1;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE 1 = f1;", false); // value-to-field
	helper(&db, "SELECT * FROM V_M_1 WHERE 1 > f1;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE 1 >= f1;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE 1 < f1;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE 1 <= f1;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE 1 <> f1;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE f1 = f1;", false); // field-to-field
	helper(&db, "SELECT * FROM V_M_1 WHERE f1 > f1;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE f1 >= f1;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE f1 < f1;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE f1 <= f1;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE f1 <> f1;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE 1 = 1;", false); // value-to-value
	helper(&db, "SELECT * FROM V_M_1 WHERE 1 > 1;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE 1 >= 1;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE 1 < 1;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE 1 <= 1;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE 1 <> 1;", false);
	// double
	helper(&db, "SELECT * FROM V_M_1 WHERE f2 = 1.0;", false); // field-to-value
	helper(&db, "SELECT * FROM V_M_1 WHERE f2 > 1.0;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE f2 >= 1.0;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE f2 < 1.0;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE f2 <= 1.0;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE f2 <> 1.0;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE 1.0 = f2;", false); // value-to-field
	helper(&db, "SELECT * FROM V_M_1 WHERE 1.0 > f2;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE 1.0 >= f2;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE 1.0 < f2;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE 1.0 <= f2;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE 1.0 <> f2;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE f2 = f2;", false); // field-to-field
	helper(&db, "SELECT * FROM V_M_1 WHERE f2 > f2;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE f2 >= f2;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE f2 < f2;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE f2 <= f2;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE f2 <> f2;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE 1.0 = 1.0;", false); // value-to-value
	helper(&db, "SELECT * FROM V_M_1 WHERE 1.0 > 1.0;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE 1.0 >= 1.0;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE 1.0 < 1.0;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE 1.0 <= 1.0;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE 1.0 <> 1.0;", false);
	// string
	helper(&db, "SELECT * FROM V_M_1 WHERE f3 = 'test';", false); // field-to-value
	helper(&db, "SELECT * FROM V_M_1 WHERE f3 <> 'test';", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE 'test' = f3;", false); // value-to-field
	helper(&db, "SELECT * FROM V_M_1 WHERE 'test' <> f3;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE f3 = f3;", false); // field-to-field
	helper(&db, "SELECT * FROM V_M_1 WHERE f3 <> f3;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE 'test' = 'test';", false); // value-to-value
	helper(&db, "SELECT * FROM V_M_1 WHERE 'test' <> 'test';", false);
	// null
	helper(&db, "SELECT * FROM V_M_1 WHERE f1 = NULL;", false); // field-to-value
	helper(&db, "SELECT * FROM V_M_1 WHERE f1 <> NULL;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE NULL = f1;", false); // value-to-field
	helper(&db, "SELECT * FROM V_M_1 WHERE NULL <> f1;", false);
	helper(&db, "SELECT * FROM V_M_1 WHERE NULL = NULL;", false); // value-to-value
	helper(&db, "SELECT * FROM V_M_1 WHERE NULL <> NULL;", false);

	/* invalid case - table doesn't exist */
	prep_helper(&db, "CREATE TABLE I_A_1 (f1 INT);");
	prep_helper(&db, "CREATE TABLE I_A_2 (f2 INT);");
	prep_helper(&db, "CREATE TABLE I_A_3 (f3 INT);");
	helper(&db, "SELECT f1 FROM I_A;", true);
	helper(&db, "SELECT f1, f2, f3 FROM I_A_1, I_A_2, I_A_31;", true);
	helper(&db, "SELECT * FROM I_A_1 JOIN I_A_2 ON f1 = f2 JOIN I_A_31 ON f2 = f3;", true);

	/* invalid case - table aliases duplicated */
	prep_helper(&db, "CREATE TABLE I_B_1 (f1 INT);");
	prep_helper(&db, "CREATE TABLE I_B_2 (f2 INT);");
	prep_helper(&db, "CREATE TABLE I_B_3 (f2 INT);");
	helper(&db, "SELECT v1.f1, v2.f2, f3 FROM I_B_1 as v1, I_B_2 as v1, I_B_3;", true);
	helper(&db, "SELECT * FROM I_B_1 v1 JOIN I_B_2 v1 ON v2.f1 = v2.f2 JOIN I_B_3 ON v2.f2 = f3;", true);

	/* invalid case - invalid column aliases */
	prep_helper(&db, "CREATE TABLE I_C_1 (f1 INT);");
	prep_helper(&db, "CREATE TABLE I_C_2 (f2 INT);");
	helper(&db, "SELECT f1 as x FROM I_C_1 as x;", true); // conflicting aliases (table <-> column)
	helper(&db, "SELECT f1 as x, f2 as x FROM I_C_1, I_C_2;", true); // duplicate aliases
	helper(&db, "SELECT f1 / 2 as val, val * 2 FROM I_C_1;", true); // reusing alias across columns
	helper(&db, "SELECT f1 as val, val * 2 FROM I_C_1;", true); // reuse alias across columns (invalid)
	helper(&db, "SELECT f1 as val, val * 2 as bla  FROM I_C_1;", true); // reuse alias across columns (invalid)

	helper(&db, "SELECT y.f1 as z FROM I_C_1 as x;", true); // invalid aliases
	helper(&db, "SELECT x.f1 as x FROM I_C_1 as x;", true); // conflicting aliases (table <-> column)
	helper(&db, "SELECT I_C_1.f1 as x, I_C_2.f2 as x FROM I_C_1, I_C_2;", true); // duplicate aliases
	helper(&db, "SELECT I_C_1.f1 / 2 as val, val * 2 FROM I_C_1;", true); // reusing alias across columns
	helper(&db, "SELECT I_C_1.f1 as val, val * 2 FROM I_C_1;", true); // reuse alias across columns (invalid)
	helper(&db, "SELECT I_C_1.f1 as val, val * 2 as bla  FROM I_C_1;", true); // reuse alias across columns (invalid)
	helper(&db, "SELECT y.f1 as val, val * 2 as bla  FROM I_C_1 y;", true); // reuse alias across columns (invalid)

	/* invalid case - column name */
	prep_helper(&db, "CREATE TABLE I_D_1 (f1 INT);");
	prep_helper(&db, "CREATE TABLE I_D_2 (f2 INT);");
	prep_helper(&db, "CREATE TABLE I_D_3 (f1 INT);");
	helper(&db, "SELECT f50 FROM I_D_1;", true);
	helper(&db, "SELECT f1, f2 FROM I_D_1, I_D_2;", false);
	helper(&db, "SELECT f1, f2 FROM I_D_1, I_D_2, I_D_3;", true);

	helper(&db, "SELECT f1 like '1' FROM I_D_1;", true); // invalid uses of expr
	helper(&db, "SELECT f1 IS NULL FROM I_D_1;", true);
	helper(&db, "SELECT f1 IN (1,2,3) FROM I_D_1;", true);

	helper(&db, "SELECT f1 FROM I_D_1 WHERE f2 > 2;", true); // no such column
	helper(&db, "SELECT a.f2 FROM I_D_1 as a WHERE a.f1 > 2;", true); // no such column
	helper(&db, "SELECT * FROM I_D_1, I_D_3 WHERE f1 > 1;", true); // ambiguous column (exprname)
	helper(&db, "SELECT a.f1 FROM I_D_1 as a WHERE a.f2 > 2;", true); // no such column (fieldname)
	helper(&db, "SELECT I_D_1.f1 FROM I_D_1 WHERE I_D_1.f2 > 2;", true); // no such column (fieldname)

//	helper(&db, "SELECT f2 FROM I_D_1 JOIN I_D_2 ON f1 = f3;", true);
//	helper(&db, "SELECT f2 FROM I_D_1 JOIN I_D_3 ON f1 = f1;", true);
//	helper(&db, "SELECT f2 FROM I_D_1 JOIN I_D_2 ON f1 = f1;", true); // f1 exists on I_D_1 but not on I_D_2

	/* invalid case - where clause */
	prep_helper(&db, "CREATE TABLE I_E_1 (f1 INT);");
	helper(&db, "SELECT f1 FROM I_E_1 WHERE 2;", true); // raw values can't be used in here
	helper(&db, "SELECT f1 FROM I_E_1 WHERE f1 + 2;", true); // recursive expr can't be used in here
	helper(&db, "SELECT f1 FROM I_E_1 WHERE 1 AND 1;", true); // raw values
	helper(&db, "SELECT f1 FROM I_E_1 WHERE 1 + 1 AND 1 + 1;", true); // raw values
	helper(&db, "SELECT f1 FROM I_E_1 WHERE f1 = 1 AND 1 + 1;", true); // raw values
	helper(&db, "SELECT f1 FROM I_E_1 WHERE f1 = 1 AND f1 = 2 OR 3;", true); // raw values
	helper(&db, "SELECT f1 FROM I_E_1 WHERE f1 = 1 AND 1 like 'asd';", true); // LIKE function
	helper(&db, "SELECT f1 FROM I_E_1 WHERE 1 like 1;", true); // LIKE function
	helper(&db, "SELECT f1 FROM I_E_1 WHERE 1 like f1;", true); // LIKE function
	helper(&db, "SELECT f1 FROM I_E_1 WHERE f1 like f1;", true); // LIKE function
	helper(&db, "SELECT f1 FROM I_E_1 WHERE f1 like 1;", true); // LIKE function
	helper(&db, "SELECT f1 FROM V_E_2 WHERE (3 * (f1 = 5)) = 10;", true); // no autoboxing from bool to int

	/* invalid case - group-by clause */
	prep_helper(&db, "CREATE TABLE I_F_1 (f1 INT);");
	prep_helper(&db, "CREATE TABLE I_F_2 (f2 INT);");
	prep_helper(&db, "CREATE TABLE I_F_3 (f1 INT);");
	prep_helper(&db, "CREATE TABLE I_F_4 (f4 INT, f5 INT);");
	helper(&db, "SELECT f1 FROM I_F_1 GROUP BY f2;", true); // no such column
	helper(&db, "SELECT x.f1 FROM I_F_1 as x GROUP BY x.f2;", true); // no such column
	helper(&db, "SELECT * FROM I_F_1, I_F_3 GROUP BY f1;", true); // ambiguous column
	helper(&db, "SELECT f1 FROM I_F_1 GROUP BY 2;", true); // raw values can't be used in here
	helper(&db, "SELECT f1 FROM I_F_1 GROUP BY f1 + 2;", true); // recursive expr can't be used in here
	helper(&db, "SELECT x.f1 FROM I_F_1 as x GROUP BY x.f1 + 2;", true); // recursive expr can't be used in here
	helper(&db, "SELECT * FROM I_F_1 JOIN I_F_3 ON f1 = f1 GROUP BY f1;", true); // ambiguous column
	helper(&db, "SELECT f1 FROM I_F_1 GROUP BY I_F_2.f1;", true);
	helper(&db, "SELECT f1 FROM I_F_1 GROUP BY I_F_1.f1;", true); // the DB is mine so I choose not to support it =)
	helper(&db, "SELECT f4 FROM I_F_4 GROUP BY f5;", true); // group-by field must be part of select fields
	helper(&db, "SELECT f4, f5 FROM I_F_4 GROUP BY f5;", true); // f4 is a non-aggregate field
	helper(&db, "SELECT I_F_4.f4 FROM I_F_4 GROUP BY I_F_4.f5;", true);

	/* invalid case - order-by clause */
	prep_helper(&db, "CREATE TABLE I_G_1 (f1 INT);");
	prep_helper(&db, "CREATE TABLE I_G_2 (f2 INT);");
	prep_helper(&db, "CREATE TABLE I_G_3 (f1 INT);");
	prep_helper(&db, "CREATE TABLE I_G_4 (f4 INT, f5 INT);");
	helper(&db, "SELECT f1 FROM I_G_1 ORDER BY f2;", true); // no such column
	helper(&db, "SELECT x.f1 FROM I_G_1 as x ORDER BY x.f2;", true); // no such column
	helper(&db, "SELECT * FROM I_G_1, I_G_3 ORDER BY f1;", true); // ambiguous column
	helper(&db, "SELECT f1 FROM I_G_1 ORDER BY 2;", true); // raw values can't be used in here
	helper(&db, "SELECT f1 FROM I_G_1 ORDER BY f2 + 2;", true); // recursive expr can't be used in here
	helper(&db, "SELECT x.f1 FROM I_G_1 as x ORDER BY x.f1 + 2;", true); // recursive expr can't be used in here
	helper(&db, "SELECT * FROM I_G_1 JOIN I_G_3 ON f1 = f1 ORDER BY f1;", true); // ambiguous column
	helper(&db, "SELECT f1 FROM I_G_1 ORDER BY I_G_1.f1;", true); // the DB is mine so I choose not to support it =)
	helper(&db, "SELECT f4 FROM I_G_4 ORDER BY f5;", true); // order-by field must be part of select fields
	helper(&db, "SELECT I_G_4.f4 FROM I_G_4 GROUP BY I_G_4.f5;", true);

	/* invalid case - COUNT function */
	prep_helper(&db, "CREATE TABLE I_H_1 (f1 INT);");
	prep_helper(&db, "CREATE TABLE I_H_2 (f2 INT, f3 INT, f4 INT);");
	helper(&db, "SELECT COUNT(*) FROM I_H_1 WHERE COUNT(*) > 1;", true); // count in where
	helper(&db, "SELECT COUNT(*) as val FROM I_H_1 WHERE val > 1;", true); // count in where
	helper(&db, "SELECT COUNT(*) FROM I_H_1 GROUP BY COUNT(*);", true); // count in group-by
	helper(&db, "SELECT COUNT(*) as val FROM I_H_1 GROUP BY val;", true); // count in group-by
	helper(&db, "SELECT COUNT(*) FROM I_H_1 ORDER BY COUNT(*);", true); // count in order-by
	helper(&db, "SELECT COUNT(*) as val FROM I_H_1 ORDER BY val;", true); // count in order-by
	helper(&db, "SELECT COUNT(1) FROM I_H_1;", true); // raw value
	helper(&db, "SELECT COUNT(1.0) FROM I_H_1;", true); // raw value
	helper(&db, "SELECT COUNT('a') FROM I_H_1;", true); // raw value
	helper(&db, "SELECT COUNT(f3) FROM I_H_1;", true); // no such column
	helper(&db, "SELECT COUNT(f1 + f2) FROM I_H_1;", true); // can't use LOGOP
	helper(&db, "SELECT COUNT(V_H_1.f1) FROM I_H_1;", true); // table isn't part of the FROM clause
	helper(&db, "SELECT COUNT(x.f1) FROM I_H_1 as y;", true); // invalid alias

	helper(&db, "SELECT COUNT(*) FROM I_H_1 HAVING COUNT(1) > 0;", true); // raw value
	helper(&db, "SELECT COUNT(*) FROM I_H_1 HAVING COUNT(1.0) > 0;", true); // raw value
	helper(&db, "SELECT COUNT(*) FROM I_H_1 HAVING COUNT('a') > 0;", true); // raw value
	helper(&db, "SELECT COUNT(*) FROM I_H_1 HAVING COUNT(f3) > 0;", true); // no such column
	helper(&db, "SELECT COUNT(*) FROM I_H_1 HAVING COUNT(f1 + f2) > 0;", true); // can't use LOGOP
	helper(&db, "SELECT COUNT(*) FROM I_H_1 HAVING COUNT(V_H_1.f1) > 0;", true); // table isn't part of the FROM clause
	helper(&db, "SELECT COUNT(*) FROM I_H_1 as y HAVING COUNT(x.f1) > 0;", true); // invalid alias

	helper(&db, "SELECT COUNT(1) FROM I_H_1 HAVING COUNT(1) > 0;", true); // raw value
	helper(&db, "SELECT COUNT(1.0) FROM I_H_1 HAVING COUNT(1.0) > 0;", true); // raw value
	helper(&db, "SELECT COUNT('a') FROM I_H_1 HAVING COUNT('a') > 0;", true); // raw value
	helper(&db, "SELECT COUNT(f3) FROM I_H_1 HAVING COUNT(f3) > 0;", true); // no such column
	helper(&db, "SELECT COUNT(f1 + f2) FROM I_H_1 HAVING COUNT(f1 + f2) > 0;", true); // can't use LOGOP
	helper(&db, "SELECT COUNT(V_H_1.f1) FROM I_H_1 HAVING COUNT(V_H_1.f1) > 0;", true); // table isn't part of the FROM clause
	helper(&db, "SELECT COUNT(x.f1) FROM I_H_1 as y HAVING COUNT(x.f1) > 0;", true); // invalid alias
	helper(&db, "SELECT COUNT(f1) + 1 FROM I_H_1;", true);
	helper(&db, "SELECT COUNT(f1) + 1 as val FROM I_H_1;", true);
	helper(&db, "SELECT (COUNT(f1) + 1) * 2 as val FROM I_H_1;", true);
	helper(&db, "SELECT (COUNT(f1) + 1) * 2 FROM I_H_1;", true);

	helper(&db, "SELECT f2, COUNT(f3) FROM I_H_2;", true); // f2 is non-aggregated field (unless group by is added
	helper(&db, "SELECT f2, COUNT(f3) as val FROM I_H_2;", true);
	helper(&db, "SELECT f2, COUNT(f3) FROM I_H_2 GROUP BY f2;", false); // Should be okay!
	helper(&db, "SELECT f2, COUNT(f3) as val FROM I_H_2 GROUP BY f2;", false);
	helper(&db, "SELECT f2, COUNT(f3), f4 FROM I_H_2 GROUP BY f4;", true); // f2 is non-aggregated field
	helper(&db, "SELECT f2, COUNT(f3) as val, f4 FROM I_H_2 GROUP BY f4;", true); // f2 is non-aggregated field

	/* invalid case - HAVING function */
	prep_helper(&db, "CREATE TABLE I_I_1 (f1 INT, f2 INT, f3 INT);");
	helper(&db, "SELECT f1 as x FROM I_I_1 HAVING f2 > 0;", true); // f2 is not part of the SELECT list
	helper(&db, "SELECT f1 as x FROM I_I_1 HAVING f1 > 0 AND f3 > 0;", true); // f3 is not part of the SELECT list
	helper(&db, "SELECT f1 FROM I_I_1 HAVING 1;", true);
	helper(&db, "SELECT f1 FROM I_I_1 HAVING f1 + 1;", true);
	helper(&db, "SELECT f1 FROM I_I_1 HAVING f1 = f2;", true);
	helper(&db, "SELECT f1 as x FROM I_I_1 HAVING y > 0;", true);

	/* invalid case - JOIN expressions */
	prep_helper(&db, "CREATE TABLE I_J_1 (f1 INT);");
	prep_helper(&db, "CREATE TABLE I_J_2 (f2 INT);");
	prep_helper(&db, "CREATE TABLE I_J_3 (f3 INT);");
	// single join
	helper(&db, "SELECT * FROM I_J_1 JOIN I_J_2 ON f1 + f3;", true); // invalid expression
	helper(&db, "SELECT * FROM I_J_1 JOIN I_J_2 ON 1 + 3;", true);
	helper(&db, "SELECT * FROM I_J_1 JOIN I_J_2 ON 1;", true);
	helper(&db, "SELECT * FROM I_J_1 JOIN I_J_2 ON COUNT(f1) > 1;", true); // COUNT is not valid here
	helper(&db, "SELECT * FROM I_J_1 JOIN I_J_2 ON COUNT(*) > 1;", true);

	helper(&db, "SELECT * FROM I_J_1 JOIN I_J_2 ON f1 = f3;", true); // no such column
	helper(&db, "SELECT * FROM I_J_1 JOIN I_J_2 ON f1 = f2 JOIN I_J_3 ON f2 = f4;", true); // no such column
	helper(&db, "SELECT * FROM I_J_1 v1 JOIN I_J_2 v2 ON v1.f1 = v2.f3 JOIN I_J_3 ON v2.f2 = f3;", true); // no such column
	helper(&db, "SELECT * FROM I_J_1 v1 JOIN I_J_2 v2 ON v1.f1 = v2.f2 JOIN I_J_3 ON v2.f2 = f4;", true); // no such column
	helper(&db, "SELECT * FROM I_J_1 v1 JOIN I_J_2 v2 ON I_J_1.f1 = I_J_2.f2;", true); // after alias is created, we can't use fqfield

	/* invalid case - ISXIN expressions */
	prep_helper(&db, "CREATE TABLE I_K_1 (f1 INT, f2 VARCHAR(1), f3 DOUBLE, f4 TINYINT, f5 DATE);");
	helper(&db, "SELECT * FROM I_K_1 WHERE f1 IN (1,2,f1);", true); // field
	helper(&db, "SELECT * FROM I_K_1 WHERE f1 IN (1,2, 1 + 1);", true); // recursive expression
	helper(&db, "SELECT * FROM I_K_1 WHERE f1 IN (1,2,'3');", true);
	helper(&db, "SELECT * FROM I_K_1 WHERE f1 NOT IN (1,2,'3');", true);
	helper(&db, "SELECT * FROM I_K_1 WHERE f2 IN ('1','2',3);", true);
	helper(&db, "SELECT * FROM I_K_1 WHERE f2 NOT IN ('1',2,'3');", true);
	helper(&db, "SELECT * FROM I_K_1 WHERE f3 IN (1.0, 2.0, 3);", true);
	helper(&db, "SELECT * FROM I_K_1 WHERE f3 NOT IN (1, 2.0, 3.0);", true);
	helper(&db, "SELECT * FROM I_K_1 WHERE f4 IN (1, 0);", true); // no auto-boxing
	helper(&db, "SELECT * FROM I_K_1 WHERE f5 IN ('20231114');", true); // date didn't parse
	helper(&db, "SELECT * FROM I_K_1 WHERE f5 IN ('2023-11-14');", false); // date parses

	/* invalid case - NULL comparison */
	prep_helper(&db, "CREATE TABLE I_L_1 (f1 INT);");
	helper(&db, "SELECT * FROM I_L_1 WHERE f1 > NULL;", true); // field-to-value
	helper(&db, "SELECT * FROM I_L_1 WHERE f1 >= NULL;", true);
	helper(&db, "SELECT * FROM I_L_1 WHERE f1 < NULL;", true);
	helper(&db, "SELECT * FROM I_L_1 WHERE f1 <= NULL;", true);
	helper(&db, "SELECT * FROM I_L_1 WHERE NULL > f1;", true); // value-to-field
	helper(&db, "SELECT * FROM I_L_1 WHERE NULL >= f1;", true);
	helper(&db, "SELECT * FROM I_L_1 WHERE NULL < f1;", true);
	helper(&db, "SELECT * FROM I_L_1 WHERE NULL <= f1;", true);
	helper(&db, "SELECT * FROM I_L_1 WHERE NULL > NULL;", true); // value-to-value
	helper(&db, "SELECT * FROM I_L_1 WHERE NULL >= NULL;", true);
	helper(&db, "SELECT * FROM I_L_1 WHERE NULL < NULL;", true);
	helper(&db, "SELECT * FROM I_L_1 WHERE NULL <= NULL;", true);

	helper(&db, "SELECT * FROM I_L_1 WHERE I_L_2.f1 IS NULL;", true); // no such table
	helper(&db, "SELECT * FROM I_L_1 WHERE I_L_1.f2 IS NULL;", true); // no such field
	helper(&db, "SELECT * FROM I_L_1 a WHERE b.f2 IS NULL;", true); // no such alias

	/* invalid case - single condition; [field|value] cmp [field|value] */
	prep_helper(&db, "CREATE TABLE I_M_1 (f1 INT, f2 DOUBLE, f3 VARCHAR(10));");
	helper(&db, "SELECT * FROM I_M_1 WHERE f3 > 'a';", true); // field-to-value
	helper(&db, "SELECT * FROM I_M_1 WHERE f3 >= 'a';", true);
	helper(&db, "SELECT * FROM I_M_1 WHERE f3 < 'a';", true);
	helper(&db, "SELECT * FROM I_M_1 WHERE f3 <= 'a';", true);
	helper(&db, "SELECT * FROM I_M_1 WHERE 'a' > f3;", true); // value-to-field
	helper(&db, "SELECT * FROM I_M_1 WHERE 'a' >= f3;", true);
	helper(&db, "SELECT * FROM I_M_1 WHERE 'a' < f3;", true);
	helper(&db, "SELECT * FROM I_M_1 WHERE 'a' <= f3;", true);
	helper(&db, "SELECT * FROM I_M_1 WHERE 'a' > 'a';", true); // value-to-value
	helper(&db, "SELECT * FROM I_M_1 WHERE 'a' >= 'a';", true);
	helper(&db, "SELECT * FROM I_M_1 WHERE 'a' < 'a';", true);
	helper(&db, "SELECT * FROM I_M_1 WHERE 'a' <= 'a';", true);

	database_close(&db);
}

void test_semantic_analyze(void)
{
	create_tests();
	insert_tests();
	delete_tests();
	update_tests();
	select_tests();
}
