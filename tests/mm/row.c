/*
 * table.c
 *
 *  Created on: 7/03/2023
 *      Author: paulo
 */

#include "tests/mm.h"
#include "mm/table.h"

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

static struct row* fetch_row(struct table *table, size_t row_num, size_t data_len)
{
	BUG_ON(data_len != table_calc_row_data_size(table));

	//TODO implement jump to next datablock when EOB is found
	struct list_head *pos;
	size_t i = 0;
	list_for_each(pos, table->datablock_head)
	{
		struct datablock *block = list_entry(pos, typeof(*block), head);

		for (size_t j = 0; j < ARR_SIZE(block->data) / table_calc_row_size(table); j++) {
			struct row *row = (struct row*)&block->data[j * table_calc_row_size(table)];
			if (i == row_num)
				return row;
			i++;
		}
	}

	return NULL;
}

static bool check_row_data(struct table *table, size_t row_num, void *expected, size_t data_len)
{
	BUG_ON(data_len != table_calc_row_data_size(table));
	struct row *row = fetch_row(table, row_num, data_len);

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

static struct row_header header_empty = {.deleted = false, .empty = true};
static struct row_header header_deleted = {.deleted = true, .empty = false};
static struct row_header header_used = {.deleted = false, .empty = false};

static bool check_row_header(struct table *table, size_t row_num, struct row_header *expected)
{
	struct row *row = fetch_row(table, row_num, table_calc_row_data_size(table));
	return memcmp(&row->header, expected, sizeof(struct row_header)) == 0;
}

static bool check_row(struct table *table, size_t row_num, struct row_header *exp_hdr, void *exp_data, size_t data_len)
{
	return check_row_header(table, row_num, exp_hdr) && check_row_data(table, row_num, exp_data, data_len);
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

static void create_test_table(struct table **out, enum COLUMN_TYPE type, size_t precision, size_t column_count)
{
	struct table *table;
	struct column column;

	table = table_init("test");

	for (size_t i = 0; i < column_count; i++) {
		snprintf(column.name, TABLE_MAX_COLUMN_NAME, "column_%zu", i);
		column.type = type;
		column.precision = precision;
		CU_ASSERT(table_add_column(table, &column));
		CU_ASSERT_EQUAL(table->column_count, i + 1);
	}

	*out = table;
}

static void create_test_table_fixed_precision_columns(struct table **out, size_t column_count)
{
	create_test_table(out, INTEGER, sizeof(int), column_count);
}

static void create_test_table_var_precision_columns(struct table **out, size_t column_precision, size_t column_count)
{
	create_test_table(out, VARCHAR, column_precision, column_count);
}

void test_table_insert_row(void)
{
	struct table *table;
	int data[] = {1, 2, 3};
	size_t fit_in_datablock = (DATABLOCK_PAGE_SIZE / table_calc_row_size(table));

	/* valid case - empty table */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(data));
	CU_ASSERT_EQUAL(count_datablocks(table), 0);
	CU_ASSERT(table_destroy(&table));

	/* valid case - normal case */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(data));
	CU_ASSERT(table_insert_row(table, data, sizeof(data)));
	CU_ASSERT(table_insert_row(table, data, sizeof(data)));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_row(table, 0, &header_used, data, sizeof(data)));
	CU_ASSERT(check_row(table, 1, &header_used, data, sizeof(data)));
	CU_ASSERT(check_row_header(table, 2, &header_empty));
	CU_ASSERT(table_destroy(&table));

	/* valid case - check if free_dtbkl_offset is moving as expected */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(data));
	CU_ASSERT(table_insert_row(table, data, sizeof(data)));
	CU_ASSERT(table_insert_row(table, data, sizeof(data)));
	CU_ASSERT(table_insert_row(table, data, sizeof(data)));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_row(table, 0, &header_used, data, sizeof(data)));
	CU_ASSERT(check_row(table, 1, &header_used, data, sizeof(data)));
	CU_ASSERT(check_row(table, 2, &header_used, data, sizeof(data)));
	CU_ASSERT(check_row_header(table, 3, &header_empty));

	CU_ASSERT(table_delete_row(table,
			fetch_datablock(table, 0),
			table_calc_row_size(table) /* index -> 1*/)))
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(check_row(table, 0, &header_used, data, sizeof(data)));
	CU_ASSERT(check_row(table, 1, &header_deleted, data, sizeof(data)));
	CU_ASSERT(check_row(table, 2, &header_used, data, sizeof(data)));
	CU_ASSERT(check_row_header(table, 3, &header_empty));

	CU_ASSERT(table_destroy(&table));

	/* valid case - force allocate new data blocks */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(data));

	for (size_t i = 0; i < fit_in_datablock * 3; i++) {
		CU_ASSERT(table_insert_row(table, data, sizeof(data)));
		CU_ASSERT(check_row(table, i, &header_used, data, sizeof(data)));
	}
	CU_ASSERT_EQUAL(count_datablocks(table), 3);
	CU_ASSERT(table_destroy(&table));

	/* invalid case - different row size */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(data));
	CU_ASSERT_FALSE(table_insert_row(table, data, sizeof(data) + 1));
	CU_ASSERT_EQUAL(count_datablocks(table), 0);
	CU_ASSERT(table_destroy(&table));

	/* invalid case - invalid row size */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(data));
	CU_ASSERT_FALSE(table_insert_row(table, data, 0));
	CU_ASSERT_EQUAL(count_datablocks(table), 0);
	CU_ASSERT(table_destroy(&table));

}

void test_table_delete_row(void)
{
	struct table *table;
	int data[] = {1, 2, 3};
	struct row *row;

	/* valid case - common */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(data));
	CU_ASSERT_EQUAL(count_datablocks(table), 0);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 0);

	CU_ASSERT(table_insert_row(table, data, sizeof(data)));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, table_calc_row_size(table));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_row(table, 0, &header_used, data, sizeof(data)));
	CU_ASSERT(check_row_header(table, 1, &header_empty));

	CU_ASSERT(table_delete_row(table, fetch_datablock(table, 0), 0));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, table_calc_row_size(table));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_row(table, 0, &header_deleted, data, sizeof(data)));
	CU_ASSERT(check_row_header(table, 1, &header_empty));
	CU_ASSERT(table_destroy(&table));

	/* invalid case - invalid offset  */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(data));
	CU_ASSERT_EQUAL(count_datablocks(table), 0);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 0);

	CU_ASSERT(table_insert_row(table, data, sizeof(data)));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, table_calc_row_size(table));
	CU_ASSERT(check_row(table, 0, &header_used, data, sizeof(data)));
	CU_ASSERT(check_row_header(table, 1, &header_empty));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT_FALSE(table_delete_row(table, fetch_datablock(table, 0), DATABLOCK_PAGE_SIZE + 1));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, table_calc_row_size(table));
	CU_ASSERT(check_row(table, 0, &header_used, data, sizeof(data)));
	CU_ASSERT(check_row_header(table, 1, &header_empty));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(table_destroy(&table));

	/* invalid case - invalid block  */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(data));
	CU_ASSERT_EQUAL(count_datablocks(table), 0);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 0);

	CU_ASSERT_FALSE(table_delete_row(table, NULL, 0));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 0);
	CU_ASSERT_EQUAL(count_datablocks(table), 0);

	CU_ASSERT(table_destroy(&table));
}

void test_table_update_row(void)
{
	struct table *table;
	int fp_old_data[] = {1, 2, 3};
	int fp_new_data[] = {4, 5, 6};

	char *vp_old_data_1 = "test1";
	char *vp_old_data_2 = "test2";
	char *vp_old_data_3 = "test3";
	uintptr_t vp_old_data[] = {(uintptr_t)vp_old_data_1, (uintptr_t)vp_old_data_2, (uintptr_t)vp_old_data_3};

	char *vp_new_data_1 = "test4";
	char *vp_new_data_2 = "test5";
	char *vp_new_data_3 = "test6";
	uintptr_t vp_new_data[] = {(uintptr_t)vp_new_data_1, (uintptr_t)vp_new_data_2, (uintptr_t)vp_new_data_3};

	/* valid case - common */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(fp_old_data));
	CU_ASSERT_EQUAL(count_datablocks(table), 0);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 0);

	CU_ASSERT(table_insert_row(table, fp_old_data, sizeof(fp_old_data)));
	CU_ASSERT(check_row(table, 0, &header_used, fp_old_data, sizeof(fp_old_data)));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, table_calc_row_size(table));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(table_update_row(table, fetch_datablock(table, 0), 0, fp_new_data, sizeof(fp_new_data)));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, table_calc_row_size(table));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_row(table, 0, &header_used, fp_new_data, sizeof(fp_new_data)));

	CU_ASSERT(table_destroy(&table));

	/* valid case - variable precision columns */
	create_test_table_var_precision_columns(&table, 6, 3);
	CU_ASSERT_EQUAL(count_datablocks(table), 0);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 0);

	CU_ASSERT(table_insert_row(table, vp_old_data, sizeof(vp_old_data)));
	CU_ASSERT(table_insert_row(table, vp_old_data, sizeof(vp_old_data)));
	CU_ASSERT(check_row(table, 0, &header_used, vp_old_data, sizeof(vp_old_data)));
	CU_ASSERT(check_row(table, 1, &header_used, vp_old_data, sizeof(vp_old_data)));
	CU_ASSERT(check_row_header(table, 2, &header_empty));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, table_calc_row_size(table) * 2);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(table_update_row(table, fetch_datablock(table, 0), 0, vp_new_data, sizeof(vp_new_data)));
	CU_ASSERT(check_row(table, 0, &header_used, vp_new_data, sizeof(vp_new_data)));
	CU_ASSERT(check_row(table, 1, &header_used, vp_old_data, sizeof(vp_old_data)));
	CU_ASSERT(check_row_header(table, 2, &header_empty));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, table_calc_row_size(table) * 2);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(table_destroy(&table));

}
