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

	// no column definition
	CU_ASSERT_EQUAL(try_parse_stmt("INSERT INTO A VALUES (123, '456');"), 0);
	// with column definition
	CU_ASSERT_EQUAL(try_parse_stmt("INSERT INTO A (f1, f2, f3, f4) VALUES (123, '456', true, 2 + 2 * 3);"), 0);
	// multiple rows
	CU_ASSERT_EQUAL(try_parse_stmt("INSERT INTO A (f1, f2) VALUES (123, '456'),(789, '012');"), 0);
	// insert from select
	CU_ASSERT_EQUAL(try_parse_stmt("INSERT INTO A (f1, f2) SELECT s1, s2 FROM B;"), 0);
	// precedence insert_expr
	CU_ASSERT_EQUAL(try_parse_stmt("INSERT INTO A VALUES ((2 + 2) * 3, 4 * (3 + 1));"), 0);
	// insert_expr that is syntactically correct but semantically invalid
	CU_ASSERT_EQUAL(try_parse_stmt("INSERT INTO A VALUES ( 1 * 'a' - 3.0 / 0);"), 0);
	// simple insert - explicit NULL
	CU_ASSERT_EQUAL(try_parse_stmt("INSERT INTO A VALUES (NULL, 1), (NULL, NULL);"), 0);

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
	// I don't want to support that hideous MySQL feature
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("INSERT INTO A (f1, f2) VALUES (1, f1 + 1);"), 0);

	/* there is no support for built-in functions
	 * wrong in so many ways lol (but on purpose) - as COUNT is the only built-in function I have for now
	 */
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("INSERT INTO A (f1) VALUES (COUNT());"), 0);
	// bitwise operators (which are supported on expr but not on insert_expr)
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("INSERT INTO A VALUE (123 & 123 | 1)"), 0);
}

void test_delete_stmt(void)
{
	/*
	 * valid tests
	 */

	// delete all (no where clause)
	CU_ASSERT_EQUAL(try_parse_stmt("DELETE FROM A;"), 0);
	// simple condition
	CU_ASSERT_EQUAL(try_parse_stmt("DELETE FROM A WHERE id = 1;"), 0);
	CU_ASSERT_EQUAL(try_parse_stmt("DELETE FROM A WHERE 1 = id;"), 0); // yep, this is valid
	// field to field condition
	CU_ASSERT_EQUAL(try_parse_stmt("DELETE FROM A WHERE f1 = f2;"), 0);
	// complex condition
	CU_ASSERT_EQUAL(try_parse_stmt("DELETE FROM A WHERE (id = 1 AND name = 'paulo') OR "
					"(surname = 'almeida' XOR surname='midori') AND sex <> 'xablau';"),
			0);

	CU_ASSERT_EQUAL(try_parse_stmt("DELETE FROM A WHERE id = 1 OR id = 2 OR id = 3;"), 0);
	// in values
	CU_ASSERT_EQUAL(try_parse_stmt("DELETE FROM A WHERE id in (1,2,3);"), 0);
	// IS NULL
	CU_ASSERT_EQUAL(try_parse_stmt("DELETE FROM A WHERE dob is NULL;"), 0);

	/*
	 * invalid tests
	 */

	// missing table name
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("DELETE FROM;"), 0);
	// missing semi-colon
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("DELETE FROM A"), 0);
	// missing expression
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("DELETE FROM A WHERE;"), 0);
	// in select - I don't wanna support this for a toy db really. (Pull requests welcome)
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("DELETE FROM A WHERE id in (select id from A);"), 0);
	// recursive math expression
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("DELETE FROM A WHERE id = (0 + 1 * 10);"), 0);
	// bitwise operations
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("DELETE FROM A WHERE id = (0 | 1);"), 0);
	// IS BOOL
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("DELETE FROM A WHERE exists IS BOOL;"), 0);
	// between - that's just some syntax flavour without actual benefits
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("DELETE FROM A WHERE value BETWEEN 1 AND 10;"), 0);
}

void test_update_stmt(void)
{
	/*
	 * valid tests
	 */

	// update all (no where clause)
	CU_ASSERT_EQUAL(try_parse_stmt("UPDATE A SET id = 1;"), 0);
	// simple condition
	CU_ASSERT_EQUAL(try_parse_stmt("UPDATE A SET id = 42 WHERE id = 1;"), 0);
	CU_ASSERT_EQUAL(try_parse_stmt("UPDATE A SET id = 42 WHERE 1 = id;"), 0); // yep, this is valid
	// field to field condition
	CU_ASSERT_EQUAL(try_parse_stmt("UPDATE A SET id = 42 WHERE f1 = f2;"), 0);
	// complex condition
	CU_ASSERT_EQUAL(try_parse_stmt("UPDATE A SET id = 42 WHERE (id = 1 AND name = 'paulo') OR "
					"(surname = 'almeida' XOR surname='midori') AND sex <> 'xablau';"),
			0);

	CU_ASSERT_EQUAL(try_parse_stmt("UPDATE A SET id = 42 WHERE id = 1 OR id = 2 OR id = 3;"), 0);
	// in values
	CU_ASSERT_EQUAL(try_parse_stmt("UPDATE A SET id = 42 WHERE id in (1,2,3);"), 0);
	// IS NULL
	CU_ASSERT_EQUAL(try_parse_stmt("UPDATE A SET id = 42 WHERE dob is NULL;"), 0);

	/*
	 * invalid tests
	 */

	// missing table name
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("UPDATE;"), 0);
	// missing update assignment
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("UPDATE A;"), 0);
	// missing semi-colon
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("UPDATE A SET id = 1"), 0);
	// missing expression
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("UPDATE A SET id = 1 WHERE;"), 0);
	// in select - I don't wanna support this for a toy db really. (Pull requests welcome)
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("UPDATE A SET id = 1 WHERE id in (select id from A);"), 0);
	// recursive math expression
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("UPDATE A SET id = 1 WHERE id = (0 + 1 * 10);"), 0);
	// bitwise operations
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("UPDATE A SET id = 1 WHERE id = (0 | 1);"), 0);
	// IS BOOL
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("UPDATE A SET id = 1 exists IS BOOL;"), 0);
	// between - that's just some syntax flavour without actual benefits
	CU_ASSERT_NOT_EQUAL(try_parse_stmt("UPDATE A SET id = 1 WHERE value BETWEEN 1 AND 10;"), 0);
}

void test_syntax_parse(void)
{
	/* create statements */
	test_create_stmt();

	/* insert statements */
	test_insert_stmt();

	/* delete statements */
	test_delete_stmt();

	/* update statements */
	test_update_stmt();
}
