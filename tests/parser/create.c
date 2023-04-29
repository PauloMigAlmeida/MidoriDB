/*
 * create.c
 *
 *  Created on: 26/04/2023
 *      Author: paulo
 */

#include <tests/parser.h>
#include <parser/syntax.h>

void test_parser_create(void)
{
	/*
	 * valid tests
	 */

	printf("\n\n");
	// single column definition
	CU_ASSERT_EQUAL(syntax_parse("CREATE TABLE A(field INTEGER);"), 0);
	// multiple column definition
	CU_ASSERT_EQUAL(syntax_parse("CREATE TABLE A(f1 INTEGER, f2 INTEGER);"), 0);
	// case insensitive
	CU_ASSERT_EQUAL(syntax_parse("create table a(field integer);"), 0);
	// Conditional creation
	CU_ASSERT_EQUAL(syntax_parse("CREATE TABLE IF NOT EXISTS A(f1 INTEGER, f2 INTEGER);"), 0);
	// Column attributes - 1
	CU_ASSERT_EQUAL(syntax_parse("CREATE TABLE IF NOT EXISTS A ("
					"  f1 INTEGER PRIMARY KEY AUTO_INCREMENT, "
					"  f2 INT UNIQUE, "
					"  f3 DOUBLE NOT NULL, "
					"  f4 FLOAT NULL, "
					"  f5 VARCHAR(10) NULL DEFAULT 'oi' "
					");"),
			0);

	/*
	 * invalid tests
	 */

	// no column definition
	CU_ASSERT_NOT_EQUAL(syntax_parse("CREATE TABLE NAME;"), 0);
	// table name starting with number
	CU_ASSERT_NOT_EQUAL(syntax_parse("CREATE TABLE 1NAME;"), 0);
	// missing semi-colon
	CU_ASSERT_NOT_EQUAL(syntax_parse("create table a(field integer)"), 0);
	// invalid column type
	CU_ASSERT_NOT_EQUAL(syntax_parse("create table a(field bla);"), 0);
	// temporary table isn't supported
	CU_ASSERT_NOT_EQUAL(syntax_parse("create temporary table a(field integer);"), 0);

}
