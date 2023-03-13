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

void test_table_add_column(void)
{
	struct table *table;
	struct column column;

	/* valid case - normal case */
	table = table_init("test");
	CU_ASSERT_EQUAL(table->column_count, 0);
	strcpy(column.name, "column_123");
	column.type = VARCHAR;
	column.precision = 10;
	CU_ASSERT(table_add_column(table, &column));
	CU_ASSERT_EQUAL(table->column_count, 1);
	CU_ASSERT(table_destroy(&table));

	/* valid case - biggest possible name */
	table = table_init("test");
	CU_ASSERT_EQUAL(table->column_count, 0);
	for (size_t i = 0; i < ARR_SIZE(column.name) - 1; i++) {
		column.name[i] = 'a';
	}
	column.name[ARR_SIZE(column.name) - 1] = '\0';
	column.type = INTEGER;
	column.precision = 0;
	CU_ASSERT(table_add_column(table, &column));
	CU_ASSERT_EQUAL(table->column_count, 1);
	CU_ASSERT(table_destroy(&table));

	/* invalid case - invalid name */
	table = table_init("test");
	CU_ASSERT_EQUAL(table->column_count, 0);
	strcpy(column.name, "!@#$%");
	column.type = VARCHAR;
	column.precision = 10;
	CU_ASSERT_EQUAL(table->column_count, 0);
	CU_ASSERT(table_destroy(&table));

	/* invalid case - name starts with number */
	table = table_init("test");
	CU_ASSERT_EQUAL(table->column_count, 0);
	strcpy(column.name, "1column");
	column.type = VARCHAR;
	column.precision = 10;
	CU_ASSERT_EQUAL(table->column_count, 0);
	CU_ASSERT(table_destroy(&table));

	/* invalid case - name starts with underscore */
	table = table_init("test");
	CU_ASSERT_EQUAL(table->column_count, 0);
	strcpy(column.name, "_column");
	column.type = VARCHAR;
	column.precision = 10;
	CU_ASSERT_EQUAL(table->column_count, 0);
	CU_ASSERT(table_destroy(&table));

	/* invalid case - empty name */
	table = table_init("test");
	CU_ASSERT_EQUAL(table->column_count, 0);
	strcpy(column.name, "");
	column.type = VARCHAR;
	column.precision = 10;
	CU_ASSERT_EQUAL(table->column_count, 0);
	CU_ASSERT(table_destroy(&table));

	/* invalid case - invalid precision/length */
	table = table_init("test");
	CU_ASSERT_EQUAL(table->column_count, 0);
	strcpy(column.name, "oi");
	column.type = VARCHAR;
	column.precision = 0;
	CU_ASSERT_FALSE(table_add_column(table, &column));
	CU_ASSERT_EQUAL(table->column_count, 0);
	CU_ASSERT(table_destroy(&table));

	/* invalid case - invalid type -> precision combination */
	table = table_init("test");
	CU_ASSERT_EQUAL(table->column_count, 0);
	strcpy(column.name, "oi");
	column.type = INTEGER;
	column.precision = 1;
	CU_ASSERT_FALSE(table_add_column(table, &column));
	CU_ASSERT_EQUAL(table->column_count, 0);
	CU_ASSERT(table_destroy(&table));

	/* testing maximum number of columns per table */
	table = table_init("test");
	CU_ASSERT_EQUAL(table->column_count, 0);
	for (size_t i = 0; i < TABLE_MAX_COLUMNS; i++) {
		snprintf(column.name, TABLE_MAX_COLUMN_NAME, "column_%zu", i);
		column.type = INTEGER;
		column.precision = 0;
		CU_ASSERT(table_add_column(table, &column));
		CU_ASSERT_EQUAL(table->column_count, i + 1);
	}
	strcpy(column.name, "one_too_much");
	CU_ASSERT_FALSE(table_add_column(table, &column));
	CU_ASSERT_EQUAL(table->column_count, TABLE_MAX_COLUMNS);
	CU_ASSERT(table_destroy(&table));
}

void test_table_rem_column(void)
{
	struct table *table;
	struct column column;

	/* valid case - normal case */
	table = table_init("test");
	strcpy(column.name, "column_123");
	column.type = VARCHAR;
	column.precision = 10;
	CU_ASSERT(table_add_column(table, &column));
	CU_ASSERT(table_rem_column(table, &column));
	CU_ASSERT_EQUAL(table->column_count, 0);
	CU_ASSERT(table_destroy(&table));

	/* valid case - remove item from the beginning of array */
	table = table_init("test");
	for (int i = 0; i < 3; i++) {
		snprintf(column.name, TABLE_MAX_COLUMN_NAME, "column_%d", i);
		column.type = VARCHAR;
		column.precision = 10;
		CU_ASSERT(table_add_column(table, &column));
		CU_ASSERT_EQUAL(table->column_count, i + 1);
	}

	CU_ASSERT(table_rem_column(table, &table->columns[0]));
	CU_ASSERT_STRING_EQUAL(table->columns[0].name, "column_1");
	CU_ASSERT_STRING_EQUAL(table->columns[1].name, "column_2");
	CU_ASSERT_EQUAL(table->column_count, 2);
	CU_ASSERT(table_destroy(&table));

	/* valid case - remove item from the middle of array */
	table = table_init("test");
	for (int i = 0; i < 3; i++) {
		snprintf(column.name, TABLE_MAX_COLUMN_NAME, "column_%d", i);
		column.type = VARCHAR;
		column.precision = 10;
		CU_ASSERT(table_add_column(table, &column));
		CU_ASSERT_EQUAL(table->column_count, i + 1);
	}

	CU_ASSERT(table_rem_column(table, &table->columns[1]));
	CU_ASSERT_STRING_EQUAL(table->columns[0].name, "column_0");
	CU_ASSERT_STRING_EQUAL(table->columns[1].name, "column_2");
	CU_ASSERT_EQUAL(table->column_count, 2);
	CU_ASSERT(table_destroy(&table));

	/* valid case - remove item from the end of array */
	table = table_init("test");
	for (int i = 0; i < 3; i++) {
		snprintf(column.name, TABLE_MAX_COLUMN_NAME, "column_%d", i);
		column.type = VARCHAR;
		column.precision = 10;
		CU_ASSERT(table_add_column(table, &column));
		CU_ASSERT_EQUAL(table->column_count, i + 1);
	}

	CU_ASSERT(table_rem_column(table, &table->columns[2]));
	CU_ASSERT_STRING_EQUAL(table->columns[0].name, "column_0");
	CU_ASSERT_STRING_EQUAL(table->columns[1].name, "column_1");
	CU_ASSERT_EQUAL(table->column_count, 2);
	CU_ASSERT(table_destroy(&table));

	/* invalid case - column that doesn't exist */
	table = table_init("test");
	strcpy(column.name, "column_123");
	column.type = VARCHAR;
	column.precision = 10;
	CU_ASSERT(table_add_column(table, &column));
	strcpy(column.name, "column_1234"); // fake column
	CU_ASSERT_FALSE(table_rem_column(table, &column));
	CU_ASSERT_EQUAL(table->column_count, 1);
	CU_ASSERT(table_destroy(&table));

	/* invalid case - name starts with number */
	table = table_init("test");
	strcpy(column.name, "column_123");
	column.type = VARCHAR;
	column.precision = 10;
	CU_ASSERT(table_add_column(table, &column));
	strcpy(column.name, "1column"); // fake column
	CU_ASSERT_FALSE(table_rem_column(table, &column));
	CU_ASSERT_EQUAL(table->column_count, 1);
	CU_ASSERT(table_destroy(&table));

	/* invalid case - name starts with underscore */
	table = table_init("test");
	strcpy(column.name, "column_123");
	column.type = VARCHAR;
	column.precision = 10;
	CU_ASSERT(table_add_column(table, &column));
	CU_ASSERT_EQUAL(table->column_count, 1);
	strcpy(column.name, "_column"); // fake column
	CU_ASSERT_FALSE(table_rem_column(table, &column));
	CU_ASSERT_EQUAL(table->column_count, 1);
	CU_ASSERT(table_destroy(&table));

	/* invalid case - empty name */
	table = table_init("test");
	strcpy(column.name, "column_123");
	column.type = VARCHAR;
	column.precision = 10;
	CU_ASSERT(table_add_column(table, &column));
	strcpy(column.name, ""); // fake column
	CU_ASSERT_FALSE(table_rem_column(table, &column));
	CU_ASSERT_EQUAL(table->column_count, 1);
	CU_ASSERT(table_destroy(&table));
}
