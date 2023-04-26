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

	// single column definition
	CU_ASSERT_EQUAL(syntax_parse("CREATE TABLE A(field INTEGER);"), 0);
	// multiple column definition
	CU_ASSERT_EQUAL(syntax_parse("CREATE TABLE A(f1 INTEGER, f2 INTEGER);"), 0);
	// case insensitive
	CU_ASSERT_EQUAL(syntax_parse("create table a(field integer);"), 0);

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
}
