/*
 * table.c
 *
 *  Created on: 7/03/2023
 *      Author: paulo
 */

#include "tests/mm.h"
#include "mm/table.h"

static struct row_header_flags header_empty = {.deleted = false, .empty = true};
static struct row_header_flags header_used = {.deleted = false, .empty = false};

void test_table_add_column(void)
{
	struct table *table;
	struct column column;

	/* valid case - normal case */
	create_test_table_fixed_precision_columns(&table, 1);
	CU_ASSERT(table_destroy(&table));

	/* valid case - biggest possible name */
	table = table_init("test");
	CU_ASSERT_EQUAL(table->column_count, 0);
	for (size_t i = 0; i < ARR_SIZE(column.name) - 1; i++) {
		column.name[i] = 'a';
	}
	column.name[ARR_SIZE(column.name) - 1] = '\0';
	column.type = INTEGER;
	column.precision = sizeof(int);
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
	column.precision = 0;
	CU_ASSERT_FALSE(table_add_column(table, &column));
	CU_ASSERT_EQUAL(table->column_count, 0);
	CU_ASSERT(table_destroy(&table));

	/* testing maximum number of columns per table */
	create_test_table_fixed_precision_columns(&table, TABLE_MAX_COLUMNS);
	strcpy(column.name, "one_too_much");
	column.type = INTEGER;
	column.precision = sizeof(int);
	CU_ASSERT_FALSE(table_add_column(table, &column));
	CU_ASSERT_EQUAL(table->column_count, TABLE_MAX_COLUMNS);
	CU_ASSERT(table_destroy(&table));

	/* add columns on table with existing data - same datablock */
	int fp_data_before[] = {1, 2, 3};
	int fp_data_after[] = {1, 2, 3, 0};

	struct row *row_before = build_row(fp_data_before, sizeof(fp_data_before), NULL, 0);
	struct row *row_after = build_row(fp_data_after, sizeof(fp_data_after), NULL, 0);

	size_t row_before_size = struct_size(row_before, data, sizeof(fp_data_before));
	size_t row_after_size = struct_size(row_after, data, sizeof(fp_data_after));

	create_test_table_fixed_precision_columns(&table, ARR_SIZE(fp_data_before));

	CU_ASSERT(table_insert_row(table, row_before, row_before_size));
	CU_ASSERT(table_insert_row(table, row_before, row_before_size));
	CU_ASSERT(check_row(table, 0, &header_used, row_before));
	CU_ASSERT(check_row(table, 1, &header_used, row_before));
	CU_ASSERT(check_row_flags(table, 2, &header_empty));

	strcpy(column.name, "new_column");
	column.type = INTEGER;
	column.precision = sizeof(fp_data_before[0]);
	CU_ASSERT(table_add_column(table, &column));

	CU_ASSERT(table_insert_row(table, row_after, row_after_size));
	CU_ASSERT(check_row(table, 0, &header_used, row_after));
	CU_ASSERT(check_row(table, 1, &header_used, row_after));
	CU_ASSERT(check_row(table, 2, &header_used, row_after));
	CU_ASSERT(check_row_flags(table, 3, &header_empty));
	CU_ASSERT(table_destroy(&table));

	free(row_before);
	free(row_after);

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
