/*
 * utils.c
 *
 *  Created on: 31/03/2023
 *      Author: paulo
 */

#include <tests/primitive.h>
#include "lib/bit.h"

static void create_test_table(struct table **out, enum COLUMN_TYPE *type, size_t precision, int column_count)
{
	struct table *table;
	struct column column;

	table = table_init("test");
	CU_ASSERT_EQUAL(table->column_count, 0);

	for (int i = 0; i < column_count; i++) {
		snprintf(column.name, TABLE_MAX_COLUMN_NAME, "column_%d", i);
		column.type = type[i];
		column.precision = precision;
		CU_ASSERT(table_add_column(table, &column));
		CU_ASSERT_EQUAL(table->column_count, i + 1);
	}

	*out = table;

	CU_ASSERT_EQUAL(count_datablocks(table), 0);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 0);
}

void create_test_table_fixed_precision_columns(struct table **out, size_t column_count)
{
	enum COLUMN_TYPE *arr = malloc(sizeof(*arr) * column_count);

	for (size_t i = 0; i < column_count; i++)
		arr[i] = CT_INTEGER;

	create_test_table(out, arr, sizeof(int), column_count);

	free(arr);
}

void create_test_table_var_precision_columns(struct table **out, size_t column_precision, size_t column_count)
{
	enum COLUMN_TYPE *arr = malloc(sizeof(*arr) * column_count);

	for (size_t i = 0; i < column_count; i++)
		arr[i] = CT_VARCHAR;

	create_test_table(out, arr, column_precision, column_count);

	free(arr);
}

void create_test_table_mixed_precision_columns(struct table **out, size_t column_precision, size_t column_count)
{
	enum COLUMN_TYPE *arr = malloc(sizeof(*arr) * column_count);

	for (size_t i = 0; i < column_count; i++)
		arr[i] = i % 2 ? CT_VARCHAR : CT_INTEGER;

	create_test_table(out, arr, column_precision, column_count);

	free(arr);
}
