/*
 * utils.c
 *
 *  Created on: 31/03/2023
 *      Author: paulo
 */

#include "tests/mm.h"
#include "lib/bit.h"

static void create_test_table(struct table **out, enum COLUMN_TYPE *type, size_t precision, int column_count)
{
	struct table *table;
	struct column column;

	table = table_init("test");

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
		arr[i] = INTEGER;

	create_test_table(out, arr, sizeof(int), column_count);

	free(arr);
}

void create_test_table_var_precision_columns(struct table **out, size_t column_precision, size_t column_count)
{
	enum COLUMN_TYPE *arr = malloc(sizeof(*arr) * column_count);

	for (size_t i = 0; i < column_count; i++)
		arr[i] = VARCHAR;

	create_test_table(out, arr, column_precision, column_count);

	free(arr);
}

void create_test_table_mixed_precision_columns(struct table **out, size_t column_precision, size_t column_count)
{
	enum COLUMN_TYPE *arr = malloc(sizeof(*arr) * column_count);

	for (size_t i = 0; i < column_count; i++)
		arr[i] = i % 2 ? VARCHAR : INTEGER;

	create_test_table(out, arr, column_precision, column_count);

	free(arr);
}

size_t count_datablocks(struct table *table)
{
	struct list_head *pos;
	size_t ret = 0;
	list_for_each(pos, table->datablock_head)
	{
		ret++;
	}
	return ret;
}

struct row* fetch_row(struct table *table, size_t row_num)
{
	size_t row_size = table_calc_row_size(table);

	//TODO implement jump to next datablock when EOB is found
	struct list_head *pos;
	size_t i = 0;
	list_for_each(pos, table->datablock_head)
	{
		struct datablock
		*block = list_entry(pos, typeof(*block), head);

		for (size_t j = 0; j < ARR_SIZE(block->data) / row_size; j++) {
			struct row *row = (struct row*)&block->data[j * row_size];
			if (i == row_num)
				return row;
			i++;
		}
	}

	return NULL;
}

bool check_row_data(struct table *table, size_t row_num, void *expected)
{
	struct row *row = fetch_row(table, row_num);

	bool ret = true;
	size_t pos = 0;
	for (int i = 0; i < table->column_count; i++) {
		struct column *column = &table->columns[i];
		if (table_check_var_column(column)) {
			void **ptr_1 = (void**)(row->data + pos);
			void **ptr_2 = (void**)((char*)expected + pos);
			ret = ret && memcmp(*ptr_1, *ptr_2, column->precision) == 0;
			pos += sizeof(uintptr_t);
		} else {
			ret = ret && memcmp(row->data + pos, (char*)expected + pos, column->precision) == 0;
			pos += column->precision;
		}
	}
	return ret;
}

bool check_row_flags(struct table *table, size_t row_num, struct row_header_flags *expected)
{
	struct row *row = fetch_row(table, row_num);
	return memcmp(&row->flags, expected, sizeof(*expected)) == 0;
}

bool check_row(struct table *table, size_t row_num, struct row_header_flags *exp_header_flags, struct row *row)
{
	return check_row_flags(table, row_num, exp_header_flags) &&
			memcmp(row->null_bitmap, row->null_bitmap, sizeof(row->null_bitmap)) == 0
			&&
			check_row_data(table, row_num, row->data);
}

struct datablock* fetch_datablock(struct table *table, size_t idx)
{
	struct list_head *pos;
	size_t i = 0;
	list_for_each(pos, table->datablock_head)
	{
		struct datablock *block = list_entry(pos, typeof(*block), head);
		if (i == idx)
			return block;
		i++;
	}
	return NULL;
}

struct row* build_row(void *data, size_t data_len, int *null_cols_idx, size_t cols_idx_len)
{
	struct row *row;
	size_t row_size = struct_size(row, data, data_len);
	row = zalloc(row_size);

	for (size_t i = 0; i < cols_idx_len; i++) {
		bit_set(row->null_bitmap, null_cols_idx[i], sizeof(row->null_bitmap));
	}

	memcpy(row->data, data, data_len);
	return row;
}
