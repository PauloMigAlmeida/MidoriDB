/*
 * semantic.c
 *
 *  Created on: 7/06/2023
 *      Author: paulo
 */

#include <tests/parser.h>
#include <parser/semantic.h>

void helper(char *stmt, bool expect_to_fail)
{
	struct ast_node *node;
	char err_msg[1024];

	memzero(err_msg, sizeof(err_msg));

	node = build_ast(stmt);
	CU_ASSERT_PTR_NOT_NULL_FATAL(node);

	CU_ASSERT_FATAL(semantic_analyse(node, err_msg, sizeof(err_msg)) == !expect_to_fail);

	if (expect_to_fail) {
		printf("err_msg: %s\n", err_msg);
	}

	ast_free(node);
}

static void create_tests(void)
{
	/* valid case */
	helper("CREATE TABLE IF NOT EXISTS A ("
		"f1 INTEGER PRIMARY KEY AUTO_INCREMENT,"
		"f2 INT UNIQUE,"
		"f3 DOUBLE NOT NULL);",
		false);

	/* valid case - pk after column definition*/
	helper("CREATE TABLE IF NOT EXISTS A ("
		"f1 INTEGER AUTO_INCREMENT,"
		"f2 INT UNIQUE,"
		"PRIMARY KEY(f1));",
		false);

	/* valid case - idx after column definition*/
	helper("CREATE TABLE IF NOT EXISTS A ("
		"f1 INTEGER AUTO_INCREMENT,"
		"f2 INT UNIQUE,"
		"INDEX(f1));",
		false);

	/* invalid case - invalid table name*/
	helper("CREATE TABLE iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii (f1 INT);",
		true);

	/* invalid case - invalid column name*/
	helper("CREATE TABLE A (iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii INT);",
		true);

	/* invalid case - column name duplicate */
	helper("CREATE TABLE IF NOT EXISTS A ("
		"f1 INTEGER AUTO_INCREMENT,"
		"f1 INT UNIQUE,"
		"INDEX(f1));",
		true);

	/* invalid case - PK after column definition points to invalid column name */
	helper("CREATE TABLE IF NOT EXISTS A ("
		"f1 INTEGER AUTO_INCREMENT,"
		"PRIMARY KEY(f2));",
		true);

	/* invalid case - index after column definition points to invalid column name */
	helper("CREATE TABLE IF NOT EXISTS A ("
		"f1 INTEGER AUTO_INCREMENT,"
		"INDEX(f2));",
		true);
}

static void insert_tests(void)
{
	/* valid case - insert - no col_names; single row */
	helper("INSERT INTO A VALUES (123, '456');", false);

	/* valid case - insert - with col_names; multiple rows */
	helper("INSERT INTO A (f1, f2) VALUES (123, '456'), (678, '901');", false);

	/* valid case - insert - no col_names; single row; recursive math expr */
	helper("INSERT INTO A VALUES ((2 + 2) * 3, 4 * (3 + 1));", false);

	/* invalid case - insert - with col_names; multiple rows; different num of terms */
	helper("INSERT INTO A (f1, f2) VALUES (123, '456'), (678);", true);

	/* invalid case - insert - with col_names; single row; different num of terms */
	helper("INSERT INTO A (f1) VALUES (123, '456');", true);

	/* invalid case - insert - invalid table name */
	helper("INSERT INTO iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii (f1) VALUES (123);",
		true);

	/* invalid case - insert - invalid column name */
	helper("INSERT INTO A (iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii) VALUES (123);",
		true);

}

void test_semantic_analyze(void)
{
	create_tests();
	insert_tests();
}
