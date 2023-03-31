/*
 * table.c
 *
 *  Created on: 7/03/2023
 *      Author: paulo
 */

#include <primitive/table.h>
#include <tests/primitive.h>

void test_table_init(void)
{
	struct table *table;

	/* valid case - normal case */
	table = table_init("test_123");
	CU_ASSERT_PTR_NOT_NULL(table);
	CU_ASSERT_STRING_EQUAL(table->name, "test_123");
	CU_ASSERT_EQUAL(table->column_count, 0);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 0);
	CU_ASSERT(table_destroy(&table));
	CU_ASSERT_PTR_NULL(table);

	/* valid case - biggest possible name */
	char bigname[TABLE_MAX_NAME + 1];
	for (size_t i = 0; i < ARR_SIZE(bigname) - 1; i++) {
		bigname[i] = 'a';
	}
	bigname[ARR_SIZE(bigname) - 1] = '\0';
	table = table_init(bigname);
	CU_ASSERT_PTR_NOT_NULL(table);
	CU_ASSERT_STRING_EQUAL(table->name, bigname);
	CU_ASSERT_EQUAL(table->column_count, 0);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 0);
	CU_ASSERT(table_destroy(&table));
	CU_ASSERT_PTR_NULL(table);

	/* invalid case -> name contains chars other than a-zA-Z */
	table = table_init("qwerty!1234!@#$");
	CU_ASSERT_PTR_NULL(table);

	/* invalid case -> name starts with number */
	table = table_init("1test");
	CU_ASSERT_PTR_NULL(table);

	/* invalid case -> name starts with underscore */
	table = table_init("_test");
	CU_ASSERT_PTR_NULL(table);

	/* invalid case -> empty name */
	table = table_init("");
	CU_ASSERT_PTR_NULL(table);

	/* invalid case -> name is too big */
	char biggername[TABLE_MAX_NAME + 2];
	for (size_t i = 0; i < ARR_SIZE(biggername) - 1; i++) {
		biggername[i] = 'a';
	}
	biggername[ARR_SIZE(biggername) - 1] = '\0';
	table = table_init(biggername);
	CU_ASSERT_PTR_NULL(table);
}

void test_table_destroy(void)
{

}

