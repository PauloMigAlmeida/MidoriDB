/*
 * vacuum.c
 *
 *  Created on: 9/04/2023
 *      Author: paulo
 */

#include <primitive/table.h>
#include <tests/primitive.h>
#include <math.h>

static struct row_header_flags header_empty = {.deleted = false, .empty = true};
static struct row_header_flags header_used = {.deleted = false, .empty = false};
static struct row_header_flags header_deleted = {.deleted = true, .empty = false};

static void delete_even_rows(struct table *table, size_t no_rows)
{
	struct datablock *datablock;
	size_t row_size = table_calc_row_size(table);
	size_t fit_in_bkl = DATABLOCK_PAGE_SIZE / row_size;

	for (size_t i = 0; i < (size_t)ceil((double)no_rows / fit_in_bkl); i++) {
		datablock = fetch_datablock(table, i);

		for (size_t j = 0; j < fit_in_bkl; j++) {
			// is work done yet?
			if (i * fit_in_bkl + j == no_rows)
				break;

			if ((i * fit_in_bkl + j) % 2 == 0) {
				CU_ASSERT(table_delete_row(table, datablock, j * row_size));
				CU_ASSERT(check_row_flags(table, i * fit_in_bkl + j, &header_deleted));
			} else {
				CU_ASSERT(check_row_flags(table, i * fit_in_bkl + j, &header_used));
			}
		}
	}
}

static bool check_rows_after_vacuum(struct table *table, size_t no_rows, struct row *expected)
{
	BUG_ON(no_rows < 2);

	bool ret = true;
	size_t row_size = table_calc_row_size(table);
	size_t fit_in_bkl = DATABLOCK_PAGE_SIZE / row_size;
	size_t cutover = no_rows / 2;

	for (size_t i = 0; i < count_datablocks(table); i++) {
		for (size_t j = 0; j < fit_in_bkl; j++) {
			if (i * fit_in_bkl + j < cutover) {
				ret = ret && check_row(table, i * fit_in_bkl + j, &header_used, expected);
			}
			else {
				ret = ret && check_row_flags(table, i * fit_in_bkl + j, &header_empty);
			}
		}
	}
	return ret;
}

void test_table_vacuum(void)
{
	struct table *table;
	struct row *row;
	int row_size, no_rows, fit_in_bkl;

	/* vacuum table with single data block */
	create_test_table_fixed_precision_columns(&table, 3);

	int data1[] = {1, 2, 3};
	row_size = table_calc_row_size(table);
	fit_in_bkl = DATABLOCK_PAGE_SIZE / row_size;
	row = build_row(data1, sizeof(data1), NULL, 0);
	no_rows = 10;

	for (int i = 0; i < no_rows; i++) {
		CU_ASSERT(table_insert_row(table, row, row_size));
		CU_ASSERT(check_row(table, i, &header_used, row));
	}
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, (no_rows % fit_in_bkl) * row_size);

	delete_even_rows(table, no_rows);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 10 * row_size);

	CU_ASSERT(table_vacuum(table));
	CU_ASSERT(check_rows_after_vacuum(table, no_rows, row));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 5 * row_size);

	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT(check_row(table, 5, &header_used, row));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 6 * row_size);

	CU_ASSERT(table_destroy(&table));
	free(row);

	/* vacuum table with 2 datablocks which after vacuum becomes 1 datablock */
	create_test_table_fixed_precision_columns(&table, 3);

	int data2[] = {1, 2, 3};
	row = build_row(data2, sizeof(data2), NULL, 0);
	row_size = table_calc_row_size(table);
	fit_in_bkl = DATABLOCK_PAGE_SIZE / row_size;
	no_rows = fit_in_bkl + 10;

	for (int i = 0; i < no_rows; i++) {
		CU_ASSERT(table_insert_row(table, row, row_size));
		CU_ASSERT(check_row(table, i, &header_used, row));
	}
	CU_ASSERT_EQUAL(count_datablocks(table), 2);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 10 * row_size);

	delete_even_rows(table, no_rows);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 10 * row_size);
	CU_ASSERT_EQUAL(count_datablocks(table), 2);

	CU_ASSERT(table_vacuum(table));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 61 * row_size); // (int((113*1+11)/2) % 113)
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_rows_after_vacuum(table, no_rows, row));

	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT(check_row(table, 61, &header_used, row));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 62 * row_size);

	CU_ASSERT(table_destroy(&table));
	free(row);

	/* vacuum table with 3 datablocks which after vacuum becomes 2 datablock */
	create_test_table_fixed_precision_columns(&table, 3);

	int data3[] = {1, 2, 3};
	row = build_row(data3, sizeof(data3), NULL, 0);
	row_size = table_calc_row_size(table);
	fit_in_bkl = DATABLOCK_PAGE_SIZE / row_size;
	no_rows = fit_in_bkl * 2 + 10;

	for (int i = 0; i < no_rows; i++) {
		CU_ASSERT(table_insert_row(table, row, row_size));
		CU_ASSERT(check_row(table, i, &header_used, row));
	}
	CU_ASSERT_EQUAL(count_datablocks(table), 3);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 10 * row_size);

	delete_even_rows(table, no_rows);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 10 * row_size);
	CU_ASSERT_EQUAL(count_datablocks(table), 3);

	CU_ASSERT(table_vacuum(table));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 5 * row_size); // (int((113*2+10)/2) % 113)
	CU_ASSERT_EQUAL(count_datablocks(table), 2);
	CU_ASSERT(check_rows_after_vacuum(table, no_rows, row));

	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT(check_row(table, 113 + 5, &header_used, row));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 6 * row_size);

	CU_ASSERT(table_destroy(&table));
	free(row);

	/* vacuum table with 4 datablocks which after vacuum becomes 2 datablock */
	create_test_table_fixed_precision_columns(&table, 3);

	int data4[] = {1, 2, 3};
	row = build_row(data4, sizeof(data4), NULL, 0);
	row_size = table_calc_row_size(table);
	fit_in_bkl = DATABLOCK_PAGE_SIZE / row_size;
	no_rows = fit_in_bkl * 3 + 10;

	for (int i = 0; i < no_rows; i++) {
		CU_ASSERT(table_insert_row(table, row, row_size));
		CU_ASSERT(check_row(table, i, &header_used, row));
	}
	CU_ASSERT_EQUAL(count_datablocks(table), 4);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 10 * row_size);

	delete_even_rows(table, no_rows);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 10 * row_size);
	CU_ASSERT_EQUAL(count_datablocks(table), 4);

	CU_ASSERT(table_vacuum(table));

	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 61 * row_size); // (int((113*3+10)/2) % 113)
	CU_ASSERT_EQUAL(count_datablocks(table), 2);
	CU_ASSERT(check_rows_after_vacuum(table, no_rows, row));

	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT(check_row(table, 113 * 1 + 60, &header_used, row));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 62 * row_size);

	CU_ASSERT(table_destroy(&table));
	free(row);

	/* vacuum table with 10 datablocks which after vacuum becomes 5 datablock */
	create_test_table_fixed_precision_columns(&table, 3);

	int data5[] = {1, 2, 3};
	row = build_row(data5, sizeof(data5), NULL, 0);
	row_size = table_calc_row_size(table);
	fit_in_bkl = DATABLOCK_PAGE_SIZE / row_size;
	no_rows = fit_in_bkl * 9 + 10;

	for (int i = 0; i < no_rows; i++) {
		CU_ASSERT(table_insert_row(table, row, row_size));
		CU_ASSERT(check_row(table, i, &header_used, row));
	}
	CU_ASSERT_EQUAL(count_datablocks(table), 10);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 10 * row_size);

	delete_even_rows(table, no_rows);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 10 * row_size);
	CU_ASSERT_EQUAL(count_datablocks(table), 10);

	CU_ASSERT(table_vacuum(table));

	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 61 * row_size); // (int((113*9+10)/2) % 113)
	CU_ASSERT_EQUAL(count_datablocks(table), 5);
	CU_ASSERT(check_rows_after_vacuum(table, no_rows, row));

	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT(check_row(table, 113 * 4 + 61, &header_used, row));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 62 * row_size);

	CU_ASSERT(table_destroy(&table));
	free(row);
}
