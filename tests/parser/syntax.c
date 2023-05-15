/*
 * create.c
 *
 *  Created on: 26/04/2023
 *      Author: paulo
 */

#include <tests/parser.h>
#include <parser/syntax.h>

static void print_stack_content(struct stack *st)
{
	for (int i = st->idx; i >= 0; i--) {
		char *ptr = *((char**)(st->arr->data + i * sizeof(uintptr_t)));
		printf("%s\n", ptr);
	}
	printf("\n");
}

static int parse_stmt(char *in)
{
	struct stack st = {0};
	int res;

	CU_ASSERT(stack_init(&st));

	res = syntax_parse(in, &st);

	// temporary
	printf("%s\n", in);
	print_stack_content(&st);

	stack_free(&st);

	return res;
}

void test_syntax_parse(void)
{
	/*
	 * valid tests
	 */

	printf("\n\n");
	// single column definition
	CU_ASSERT_EQUAL(parse_stmt("CREATE TABLE A(field INTEGER);"), 0);
	// multiple column definition
	CU_ASSERT_EQUAL(parse_stmt("CREATE TABLE A(f1 INTEGER, f2 INTEGER);"), 0);
	// case insensitive
	CU_ASSERT_EQUAL(parse_stmt("create table a(field integer);"), 0);
	// Conditional creation
	CU_ASSERT_EQUAL(parse_stmt("CREATE TABLE IF NOT EXISTS A(f1 INTEGER, f2 INTEGER);"), 0);
	// Column attributes - 1
	CU_ASSERT_EQUAL(parse_stmt("CREATE TABLE IF NOT EXISTS A ("
					"  f1 INTEGER PRIMARY KEY AUTO_INCREMENT, "
					"  f2 INT UNIQUE, "
					"  f3 DOUBLE NOT NULL, "
					"  f4 FLOAT NULL, "
					"  f5 VARCHAR(10) NULL"
					");"
					),
			0);

	/*
	 * invalid tests
	 */

	// no column definition
	CU_ASSERT_NOT_EQUAL(parse_stmt("CREATE TABLE NAME;"), 0);
	// table name starting with number
	CU_ASSERT_NOT_EQUAL(parse_stmt("CREATE TABLE 1NAME;"), 0);
	// missing semi-colon
	CU_ASSERT_NOT_EQUAL(parse_stmt("create table a(field integer)"), 0);
	// invalid column type
	CU_ASSERT_NOT_EQUAL(parse_stmt("create table a(field bla);"), 0);
	// temporary table isn't supported
	CU_ASSERT_NOT_EQUAL(parse_stmt("create temporary table a(field integer);"), 0);
}
