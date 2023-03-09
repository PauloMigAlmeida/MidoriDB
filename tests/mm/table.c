/*
 * table.c
 *
 *  Created on: 7/03/2023
 *      Author: paulo
 */

#include "tests/mm.h"
#include "mm/table.h"

void test_table_init(void)
{
	struct table *table;

	/* valid case - normal case */
	table = table_init("test_123");
	CU_ASSERT_PTR_NOT_NULL(table);
	CU_ASSERT_STRING_EQUAL(table->name, "test_123");
	CU_ASSERT_EQUAL(table->column_count, 0);
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

void test_table_add_column(void)
{
	struct table *table;
	struct column column;

	table = table_init("test");
	CU_ASSERT_EQUAL(table->column_count, 0);

	/* valid case - normal case */
	strcpy(column.name, "column_123");
	column.length = 10;
	CU_ASSERT(table_add_column(table, &column));
	CU_ASSERT_EQUAL(table->column_count, 1);

	/* valid case - biggest possible name */
	for (size_t i = 0; i < ARR_SIZE(column.name) - 1; i++) {
		column.name[i] = 'a';
	}
	column.name[ARR_SIZE(column.name) - 1] = '\0';
	column.length = 10;
	CU_ASSERT(table_add_column(table, &column));
	CU_ASSERT_EQUAL(table->column_count, 2);

	/* invalid case - invalid name */
	strcpy(column.name, "!@#$%");
	column.length = 10;
	CU_ASSERT_FALSE(table_add_column(table, &column));
	CU_ASSERT_EQUAL(table->column_count, 2);

	/* invalid case - name starts with number*/
	strcpy(column.name, "1column");
	column.length = 10;
	CU_ASSERT_FALSE(table_add_column(table, &column));
	CU_ASSERT_EQUAL(table->column_count, 2);

	/* invalid case - name starts with underscore*/
	strcpy(column.name, "_column");
	column.length = 10;
	CU_ASSERT_FALSE(table_add_column(table, &column));
	CU_ASSERT_EQUAL(table->column_count, 2);

	/* invalid case - empty name */
	strcpy(column.name, "");
	column.length = 10;
	CU_ASSERT_FALSE(table_add_column(table, &column));
	CU_ASSERT_EQUAL(table->column_count, 2);

	/* invalid case - invalid precision/length */
	strcpy(column.name, "oi");
	column.length = 0;
	CU_ASSERT_FALSE(table_add_column(table, &column));
	CU_ASSERT_EQUAL(table->column_count, 2);

	CU_ASSERT(table_destroy(&table));
}
