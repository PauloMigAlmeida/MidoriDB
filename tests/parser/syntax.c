/*
 * create.c
 *
 *  Created on: 26/04/2023
 *      Author: paulo
 */

#include <tests/parser.h>
#include <parser/syntax.h>

static void print_queue(struct queue *ct)
{
	for (size_t i = 0; i <= (ct->arr->len / sizeof(uintptr_t)); i++) {
		printf("stack pos: %lu, content: %s\n", i, (char*)queue_peek_pos(ct, i));
	}
	printf("\n");
}

static int parse_stmt(char *in)
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
