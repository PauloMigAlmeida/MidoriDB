/*
 * table.c
 *
 *  Created on: 7/03/2023
 *      Author: paulo
 */

#include "tests/mm.h"
#include "mm/table.h"
#include "lib/bit.h"

static size_t count_datablocks(struct table *table)
{
	struct list_head *pos;
	size_t ret = 0;
	list_for_each(pos, table->datablock_head)
	{
		ret++;
	}
	return ret;
}

static struct row* fetch_row(struct table *table, size_t row_num)
{
	size_t row_size = table_calc_row_size(table);

	//TODO implement jump to next datablock when EOB is found
	struct list_head *pos;
	size_t i = 0;
	list_for_each(pos, table->datablock_head)
	{
		struct datablock *block = list_entry(pos, typeof(*block), head);

		for (size_t j = 0; j < ARR_SIZE(block->data) / row_size; j++) {
			struct row *row = (struct row*)&block->data[j * row_size];
			if (i == row_num)
				return row;
			i++;
		}
	}

	return NULL;
}

static bool check_row_data(struct table *table, size_t row_num, void *expected)
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

static struct row_header_flags header_empty = {.deleted = false, .empty = true};
static struct row_header_flags header_deleted = {.deleted = true, .empty = false};
static struct row_header_flags header_used = {.deleted = false, .empty = false};

static bool check_row_header_flags(struct table *table, size_t row_num, struct row_header_flags *expected)
{
	struct row *row = fetch_row(table, row_num);
	return memcmp(&row->header.flags, expected, sizeof(*expected)) == 0;
}

static bool check_row(struct table *table, size_t row_num, struct row_header_flags *exp_header_flags, struct row *row)
{
	return check_row_header_flags(table, row_num, exp_header_flags) &&
			memcmp(row->header.null_bitmap, row->header.null_bitmap, sizeof(row->header.null_bitmap)) == 0
			&&
			check_row_data(table, row_num, row->data);
}

static struct datablock* fetch_datablock(struct table *table, size_t idx)
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

static void create_test_table(struct table **out, enum COLUMN_TYPE type, size_t precision, int column_count)
{
	struct table *table;
	struct column column;

	table = table_init("test");

	for (int i = 0; i < column_count; i++) {
		snprintf(column.name, TABLE_MAX_COLUMN_NAME, "column_%d", i);
		column.type = type;
		column.precision = precision;
		CU_ASSERT(table_add_column(table, &column));
		CU_ASSERT_EQUAL(table->column_count, i + 1);
	}

	*out = table;

	CU_ASSERT_EQUAL(count_datablocks(table), 0);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 0);
}

static void create_test_table_fixed_precision_columns(struct table **out, size_t column_count)
{
	create_test_table(out, INTEGER, sizeof(int), column_count);
}

static void create_test_table_var_precision_columns(struct table **out, size_t column_precision, size_t column_count)
{
	create_test_table(out, VARCHAR, column_precision, column_count);
}

static struct row* build_row(void *data, size_t data_len, int *null_cols_idx, size_t cols_idx_len)
{
	struct row *row;
	size_t row_size = struct_size(row, data, data_len);
	row = zalloc(row_size);

	for (size_t i = 0; i < cols_idx_len; i++) {
		bit_set(row->header.null_bitmap, null_cols_idx[i], sizeof(row->header.null_bitmap));
	}

	memcpy(row->data, data, data_len);
	return row;
}

void test_table_insert_row(void)
{
	struct table *table;

	int data[] = {0, 1, 2, 3, 0};
	struct row *row = build_row(data, sizeof(data), NULL, 0);

	int null_fields[] = {0, 3};
	struct row *row_nulls = build_row(data, sizeof(data), null_fields, ARR_SIZE(null_fields));

	size_t row_size = struct_size(row, data, sizeof(data));
	size_t fit_in_datablock = (DATABLOCK_PAGE_SIZE / row_size);

	/* valid case - empty table */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(data));
	CU_ASSERT(table_destroy(&table));

	/* valid case - normal case */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(data));
	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_row(table, 0, &header_used, row));
	CU_ASSERT(check_row(table, 1, &header_used, row));
	CU_ASSERT(check_row_header_flags(table, 2, &header_empty));
	CU_ASSERT(table_destroy(&table));

	/* valid case - check if free_dtbkl_offset is moving as expected */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(data));
	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_row(table, 0, &header_used, row));
	CU_ASSERT(check_row(table, 1, &header_used, row));
	CU_ASSERT(check_row(table, 2, &header_used, row));
	CU_ASSERT(check_row_header_flags(table, 3, &header_empty));

	CU_ASSERT(table_delete_row(table, fetch_datablock(table, 0), row_size));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(check_row(table, 0, &header_used, row));
	CU_ASSERT(check_row(table, 1, &header_deleted, row));
	CU_ASSERT(check_row(table, 2, &header_used, row));
	CU_ASSERT(check_row_header_flags(table, 3, &header_empty));

	CU_ASSERT(table_destroy(&table));

	/* valid case - force allocate new data blocks */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(data));

	for (size_t i = 0; i < fit_in_datablock * 3; i++) {
		CU_ASSERT(table_insert_row(table, row, row_size));
		CU_ASSERT(check_row(table, i, &header_used, row));
	}
	CU_ASSERT_EQUAL(count_datablocks(table), 3);
	CU_ASSERT(table_destroy(&table));

	/* valid case - insert row with null fields */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(data));
	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT(table_insert_row(table, row_nulls, row_size));
	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_row(table, 0, &header_used, row));
	CU_ASSERT(check_row(table, 1, &header_used, row_nulls));
	CU_ASSERT(check_row(table, 2, &header_used, row));
	CU_ASSERT(check_row_header_flags(table, 3, &header_empty));
	CU_ASSERT(table_destroy(&table));

	/* invalid case - different row size */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(data));
	CU_ASSERT_FALSE(table_insert_row(table, row, row_size + 1));
	CU_ASSERT_EQUAL(count_datablocks(table), 0);
	CU_ASSERT(table_destroy(&table));

	/* invalid case - invalid row size */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(data));
	CU_ASSERT_FALSE(table_insert_row(table, row, 0));
	CU_ASSERT_EQUAL(count_datablocks(table), 0);
	CU_ASSERT(table_destroy(&table));

	free(row);
	free(row_nulls);
}

void test_table_delete_row(void)
{
	struct table *table;

	int data[] = {0, 1, 2, 3, 0};
	int null_fields[] = {0, 3};

	struct row *row = build_row(data, sizeof(data), NULL, 0);
	struct row *row_nulls = build_row(data, sizeof(data), null_fields, ARR_SIZE(null_fields));

	size_t row_size = struct_size(row, data, sizeof(data));

	/* valid case - common */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(data));

	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_row(table, 0, &header_used, row));
	CU_ASSERT(check_row_header_flags(table, 1, &header_empty));

	CU_ASSERT(table_delete_row(table, fetch_datablock(table, 0), 0));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(check_row(table, 0, &header_deleted, row));
	CU_ASSERT(check_row_header_flags(table, 1, &header_empty));
	CU_ASSERT(table_destroy(&table));

	/* valid case - row with null values */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(data));

	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT(table_insert_row(table, row_nulls, row_size));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size * 2);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_row(table, 0, &header_used, row));
	CU_ASSERT(check_row(table, 1, &header_used, row_nulls));
	CU_ASSERT(check_row_header_flags(table, 2, &header_empty));

	CU_ASSERT(table_delete_row(table, fetch_datablock(table, 0), row_size));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size * 2);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(check_row(table, 0, &header_used, row));
	CU_ASSERT(check_row(table, 1, &header_deleted, row_nulls));
	CU_ASSERT(check_row_header_flags(table, 2, &header_empty));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size * 2);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(table_destroy(&table));

	/* invalid case - invalid offset  */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(data));

	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size);
	CU_ASSERT(check_row(table, 0, &header_used, row));
	CU_ASSERT(check_row_header_flags(table, 1, &header_empty));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT_FALSE(table_delete_row(table, fetch_datablock(table, 0), DATABLOCK_PAGE_SIZE + 1));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size);
	CU_ASSERT(check_row(table, 0, &header_used, row));
	CU_ASSERT(check_row_header_flags(table, 1, &header_empty));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(table_destroy(&table));

	/* invalid case - invalid block  */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(data));
	CU_ASSERT_FALSE(table_delete_row(table, NULL, 0));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 0);
	CU_ASSERT_EQUAL(count_datablocks(table), 0);

	CU_ASSERT(table_destroy(&table));

	free(row);
}

void test_table_update_row(void)
{
	struct table *table;
	struct row *row_old, *row_new;
	size_t row_size;

	/* valid case - fixed precision columns */
	int fp_old_data[] = {1, 2, 3};
	int fp_new_data[] = {4, 5, 6};

	row_old = build_row(fp_old_data, sizeof(fp_old_data), NULL, 0);
	row_new = build_row(fp_new_data, sizeof(fp_new_data), NULL, 0);
	row_size = struct_size(row_new, data, sizeof(fp_old_data));
	
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(fp_old_data));

	CU_ASSERT(table_insert_row(table, row_old, row_size));
	CU_ASSERT(check_row(table, 0, &header_used, row_old));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(table_update_row(table, fetch_datablock(table, 0), 0, row_new, row_size));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_row(table, 0, &header_used, row_new));

	CU_ASSERT(table_destroy(&table));
	free(row_old);
	free(row_new);

	/* valid case - fixed precision columns with NULL values */
	int fpn_old_data[] = {0, 2, 0};
	int fpn_old_null_fields[] = {0, 2};

	int fpn_new_data[] = {4, 0, 6};
	int fpn_new_null_fields[] = {1};

	row_old = build_row(fpn_old_data, sizeof(fpn_old_data), fpn_old_null_fields, ARR_SIZE(fpn_old_null_fields));
	row_new = build_row(fpn_new_data, sizeof(fpn_new_data), fpn_new_null_fields, ARR_SIZE(fpn_new_null_fields));
	row_size = struct_size(row_new, data, sizeof(fpn_old_data));
	
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(fpn_old_data));

	CU_ASSERT(table_insert_row(table, row_old, row_size));
	CU_ASSERT(check_row(table, 0, &header_used, row_old));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(table_update_row(table, fetch_datablock(table, 0), 0, row_new, row_size));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_row(table, 0, &header_used, row_new));

	CU_ASSERT(table_destroy(&table));
	free(row_old);
	free(row_new);

	/* valid case - variable precision columns */
	char *vp_old_data_1 = "test1";
	char *vp_old_data_2 = "test2";
	char *vp_old_data_3 = "test3";
	uintptr_t vp_old_data[] = {(uintptr_t)vp_old_data_1, (uintptr_t)vp_old_data_2, (uintptr_t)vp_old_data_3};

	char *vp_new_data_1 = "test4";
	char *vp_new_data_2 = "test5";
	char *vp_new_data_3 = "test6";
	uintptr_t vp_new_data[] = {(uintptr_t)vp_new_data_1, (uintptr_t)vp_new_data_2, (uintptr_t)vp_new_data_3};

	row_old = build_row(vp_old_data, sizeof(vp_old_data), NULL, 0);
	row_new = build_row(vp_new_data, sizeof(vp_new_data), NULL, 0);
	row_size = struct_size(row_new, data, sizeof(vp_old_data));

	create_test_table_var_precision_columns(&table, 6, 3);

	CU_ASSERT(table_insert_row(table, row_old, row_size));
	CU_ASSERT(table_insert_row(table, row_old, row_size));
	CU_ASSERT(check_row(table, 0, &header_used, row_old));
	CU_ASSERT(check_row(table, 1, &header_used, row_old));
	CU_ASSERT(check_row_header_flags(table, 2, &header_empty));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, table_calc_row_size(table) * 2);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(table_update_row(table, fetch_datablock(table, 0), row_size, row_new, row_size));
	CU_ASSERT(check_row(table, 0, &header_used, row_old));
	CU_ASSERT(check_row(table, 1, &header_used, row_new));
	CU_ASSERT(check_row_header_flags(table, 2, &header_empty));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size * 2);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(table_destroy(&table));
	free(row_old);
	free(row_new);

	/* valid case - variable precision columns with null columns */
	char *vpn_old_data_1 = "00000";
	char *vpn_old_data_2 = "test2";
	char *vpn_old_data_3 = "00000";
	int vpn_old_null_fields[] = {0, 2};
	uintptr_t vpn_old_data[] = {(uintptr_t)vpn_old_data_1, (uintptr_t)vpn_old_data_2, (uintptr_t)vpn_old_data_3};

	char *vpn_new_data_1 = "test4";
	char *vpn_new_data_2 = "00000";
	char *vpn_new_data_3 = "test6";
	int vpn_new_null_fields[] = {1};
	uintptr_t vpn_new_data[] = {(uintptr_t)vpn_new_data_1, (uintptr_t)vpn_new_data_2, (uintptr_t)vpn_new_data_3};

	row_old = build_row(vpn_old_data, sizeof(vpn_old_data), vpn_old_null_fields, ARR_SIZE(vpn_old_null_fields));
	row_new = build_row(vpn_new_data, sizeof(vpn_new_data), vpn_new_null_fields, ARR_SIZE(vpn_new_null_fields));
	row_size = struct_size(row_new, data, sizeof(vpn_old_data));

	create_test_table_var_precision_columns(&table, 6, 3);

	CU_ASSERT(table_insert_row(table, row_old, row_size));
	CU_ASSERT(table_insert_row(table, row_old, row_size));
	CU_ASSERT(check_row(table, 0, &header_used, row_old));
	CU_ASSERT(check_row(table, 1, &header_used, row_old));
	CU_ASSERT(check_row_header_flags(table, 2, &header_empty));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, table_calc_row_size(table) * 2);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(table_update_row(table, fetch_datablock(table, 0), row_size, row_new, row_size));
	CU_ASSERT(check_row(table, 0, &header_used, row_old));
	CU_ASSERT(check_row(table, 1, &header_used, row_new));
	CU_ASSERT(check_row_header_flags(table, 2, &header_empty));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size * 2);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(table_destroy(&table));
	free(row_old);
	free(row_new);
}
