/*
 * create.c
 *
 *  Created on: 26/04/2023
 *      Author: paulo
 */

#include <tests/parser.h>
#include <parser/syntax.h>

static int try_parse_stmt(char *in)
{
	struct queue ct = {0};
	int res;

	CU_ASSERT(queue_init(&ct));

	res = syntax_parse(in, &ct);

//	 temporary
	printf("%s\n", in);
	print_queue(&ct);

	queue_free(&ct);

	return res;
}

static void test_create_stmt(void)
{

	/*
	 * valid tests
	 */

	// single column definition
	CU_ASSERT_EQUAL(try_parse_stmt("CREATE TABLE A(field INTEGER);"), 0);
	// multiple column definition
	CU_ASSERT_EQUAL(try_parse_stmt("CREATE TABLE A(f1 INTEGER, f2 INTEGER);"), 0);
	// case insensitive
	CU_ASSERT_EQUAL(try_parse_stmt("create table a(field integer);"), 0);
	// Conditional creation
	CU_ASSERT_EQUAL(try_parse_stmt("CREATE TABLE IF NOT EXISTS A(f1 INTEGER, f2 INTEGER);"), 0);
	// Column attributes - 1
	CU_ASSERT_EQUAL(try_parse_stmt("CREATE TABLE IF NOT EXISTS A ("
					"  f1 INTEGER PRIMARY KEY AUTO_INCREMENT, "
					"  f2 INT UNIQUE, "
					"  f3 DOUBLE NOT NULL, "
					"  f5 VARCHAR(10) NULL"
					");"
					),
			0);
	// defining primary key and index after column definition
	CU_ASSERT_EQUAL(try_parse_stmt("CREATE TABLE IF NOT EXISTS A ("
					"  f1 INTEGER AUTO_INCREMENT, "
					"  f2 INT UNIQUE, "
					"  f3 DOUBLE NOT NULL, "
					"  f5 VARCHAR(10) NULL,"
					"  PRIMARY KEY(f1),"
					"  INDEX(f2)"
					");"
					),
			0);

	/*
	 * invalid tests
	 */

	// no column definition
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("CREATE TABLE NAME;"), 0);
	// table name starting with number
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("CREATE TABLE 1NAME;"), 0);
	// missing semi-colon
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("create table a(field integer)"), 0);
	// invalid column type
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("create table a(field bla);"), 0);
	// temporary table isn't supported
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("create temporary table a(field integer);"), 0);
	// We don't have support for "schemas"
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("CREATE TABLE A.G (f1 INTEGER);"), 0);

}

static void test_insert_stmt(void)
{
	/*
	 * valid tests
	 */

	// simple insert - no column definition
	CU_ASSERT_EQUAL(try_parse_stmt("INSERT INTO A VALUES (123, '456');"), 0);
	// simple insert - with column definition
	CU_ASSERT_EQUAL(try_parse_stmt("INSERT INTO A (f1, f2, f3, f4) VALUES (123, '456', true, 2 + 2 * 3);"), 0);
	// simple insert - multiple rows
	CU_ASSERT_EQUAL(try_parse_stmt("INSERT INTO A (f1, f2) VALUES (123, '456'),(789, '012');"), 0);
	// insert from select
	CU_ASSERT_EQUAL(try_parse_stmt("INSERT INTO A (f1, f2) SELECT s1, s2 FROM B;"), 0);

	//TODO This is valid but should be invalid - as I don't want to support that hideous MySQL feature
	CU_ASSERT_EQUAL(try_parse_stmt("INSERT INTO A (f1, f2) VALUES (a + 1);"), 0);
	// simple insert - precedence expr
	CU_ASSERT_EQUAL(try_parse_stmt("INSERT INTO A VALUES ((2 + 2) * 3, 4 * (3 + 1));"), 0);

	/*
	 * invalid tests
	 */

	// empty column definition
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("INSERT INTO A () VALUES (123, '456');"), 0);
	// missing table name
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("INSERT INTO (f1, f2) VALUES (123, '456');"), 0);
	// missing VALUES
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("INSERT INTO A (123);"), 0);
	// missing semi-column
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("INSERT INTO A VALUE (123)"), 0);

}

void test_syntax_parse(void)
{
	printf("\n\n");
	/* create statements */
	test_create_stmt();

	/* insert statements */
	test_insert_stmt();
}
