/*
 * table.c
 *
 *  Created on: 7/03/2023
 *      Author: paulo
 */

#include <primitive/table.h>
#include <tests/primitive.h>

static struct row_header_flags header_empty = {.deleted = false, .empty = true};
static struct row_header_flags header_used = {.deleted = false, .empty = false};

void test_table_add_column(void)
{
	struct table *table;
	struct column column;

	struct row *row_before;
	struct row *row_after;

	size_t row_before_size;
	size_t row_after_size;

	size_t no_rows;

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

	/* add columns on table with existing data - fixed precision - same datablock */
	int fp_data1_before[] = {1, 2, 3};
	int fp_data1_after[] = {1, 2, 3, 0};

	row_before = build_row(fp_data1_before, sizeof(fp_data1_before), NULL, 0);
	row_after = build_row(fp_data1_after, sizeof(fp_data1_after), NULL, 0);

	row_before_size = struct_size(row_before, data, sizeof(fp_data1_before));
	row_after_size = struct_size(row_after, data, sizeof(fp_data1_after));

	create_test_table_fixed_precision_columns(&table, ARR_SIZE(fp_data1_before));

	CU_ASSERT(table_insert_row(table, row_before, row_before_size));
	CU_ASSERT(table_insert_row(table, row_before, row_before_size));
	CU_ASSERT(check_row(table, 0, &header_used, row_before));
	CU_ASSERT(check_row(table, 1, &header_used, row_before));
	CU_ASSERT(check_row_flags(table, 2, &header_empty));

	strcpy(column.name, "new_column");
	column.type = INTEGER;
	column.precision = sizeof(fp_data1_before[0]);
	CU_ASSERT(table_add_column(table, &column));

	CU_ASSERT(table_insert_row(table, row_after, row_after_size));
	CU_ASSERT(check_row(table, 0, &header_used, row_after));
	CU_ASSERT(check_row(table, 1, &header_used, row_after));
	CU_ASSERT(check_row(table, 2, &header_used, row_after));
	CU_ASSERT(check_row_flags(table, 3, &header_empty));

	// make sure we got the right number of
	// datablocks after the data rearrange process
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_after_size * 3);

	CU_ASSERT(table_destroy(&table));

	free(row_before);
	free(row_after);

	/* add columns on table with existing data - mixed precision - same datablock */
	char *vp_data1_1 = "test1";
	char *vp_data1_2 = "test2";
	char *vp_data1_3 = "test3";
	uintptr_t vp_data1_before[] = {(uintptr_t)vp_data1_1, (uintptr_t)vp_data1_2, (uintptr_t)vp_data1_3};
	uintptr_t vp_data1_after[] = {(uintptr_t)vp_data1_1, (uintptr_t)vp_data1_2, (uintptr_t)vp_data1_3, 0};

	row_before = build_row(vp_data1_before, sizeof(vp_data1_before), NULL, 0);
	row_after = build_row(vp_data1_after, sizeof(vp_data1_after), NULL, 0);

	row_before_size = struct_size(row_before, data, sizeof(vp_data1_before));
	row_after_size = struct_size(row_after, data, sizeof(vp_data1_after));

	create_test_table_var_precision_columns(&table, strlen(vp_data1_1) + 1, ARR_SIZE(vp_data1_before));

	CU_ASSERT(table_insert_row(table, row_before, row_before_size));
	CU_ASSERT(table_insert_row(table, row_before, row_before_size));
	CU_ASSERT(check_row(table, 0, &header_used, row_before));
	CU_ASSERT(check_row(table, 1, &header_used, row_before));
	CU_ASSERT(check_row_flags(table, 2, &header_empty));

	strcpy(column.name, "new_column");
	column.type = INTEGER;
	column.precision = sizeof(vp_data1_after[3]);
	CU_ASSERT(table_add_column(table, &column));

	CU_ASSERT(table_insert_row(table, row_after, row_after_size));
	CU_ASSERT(check_row(table, 0, &header_used, row_after));
	CU_ASSERT(check_row(table, 1, &header_used, row_after));
	CU_ASSERT(check_row(table, 2, &header_used, row_after));
	CU_ASSERT(check_row_flags(table, 3, &header_empty));

	// make sure we got the right number of
	// datablocks after the data rearrange process
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_after_size * 3);

	CU_ASSERT(table_destroy(&table));

	free(row_before);
	free(row_after);

	/* add columns on table with existing data - fixed precision - force the creation of a datablock */
	int fp_data2_before[] = {1, 2, 3};
	int fp_data2_after[] = {1, 2, 3, 0};

	row_before = build_row(fp_data2_before, sizeof(fp_data2_before), NULL, 0);
	row_after = build_row(fp_data2_after, sizeof(fp_data2_after), NULL, 0);

	row_before_size = struct_size(row_before, data, sizeof(fp_data2_before));
	row_after_size = struct_size(row_after, data, sizeof(fp_data2_after));

	create_test_table_fixed_precision_columns(&table, ARR_SIZE(fp_data2_before));

	no_rows = (DATABLOCK_PAGE_SIZE / row_before_size);
	for (size_t i = 0; i < no_rows; i++) {
		CU_ASSERT(table_insert_row(table, row_before, row_before_size));
		CU_ASSERT(check_row(table, i, &header_used, row_before));
	}
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	strcpy(column.name, "new_column");
	column.type = INTEGER;
	column.precision = sizeof(fp_data2_before[0]);
	CU_ASSERT(table_add_column(table, &column));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, (no_rows - (DATABLOCK_PAGE_SIZE / row_after_size)) * row_after_size);
	// make sure we got the right number of
	// datablocks after the data rearrange process
	CU_ASSERT_EQUAL(count_datablocks(table), 2);

	for (size_t i = 0; i < no_rows; i++) {
		/*
		 * the block is zeroed-out so we can anticipate which value/gap will be left
		 * when column is added. Additionally, we must ensure that the same number
		 * of rows exists in the table (though in this case the data will be spread
		 * across 2 datablocks instead of 1)
		 */
		CU_ASSERT(check_row(table, i, &header_used, row_after));
	}

	CU_ASSERT(table_insert_row(table, row_after, row_after_size));
	CU_ASSERT(check_row(table, no_rows, &header_used, row_after));
	CU_ASSERT(check_row_flags(table, no_rows + 1, &header_empty));

	CU_ASSERT_EQUAL(count_datablocks(table), 2);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset,
			(no_rows + 1 - (DATABLOCK_PAGE_SIZE / row_after_size)) * row_after_size);

	CU_ASSERT(table_destroy(&table));

	free(row_before);
	free(row_after);

	/* add columns on table with existing data - mixed precision - force the creation of a datablock */
	char *vp_data2_1 = "test1";
	char *vp_data2_2 = "test2";
	char *vp_data2_3 = "test3";
	uintptr_t vp_data2_before[] = {(uintptr_t)vp_data2_1, (uintptr_t)vp_data2_2, (uintptr_t)vp_data2_3};
	uintptr_t vp_data2_after[] = {(uintptr_t)vp_data2_1, (uintptr_t)vp_data2_2, (uintptr_t)vp_data2_3, 0};

	row_before = build_row(vp_data2_before, sizeof(vp_data2_before), NULL, 0);
	row_after = build_row(vp_data2_after, sizeof(vp_data2_after), NULL, 0);

	row_before_size = struct_size(row_before, data, sizeof(vp_data2_before));
	row_after_size = struct_size(row_after, data, sizeof(vp_data2_after));

	create_test_table_var_precision_columns(&table, strlen(vp_data2_1) + 1, ARR_SIZE(vp_data2_before));

	no_rows = (DATABLOCK_PAGE_SIZE / row_before_size);
	for (size_t i = 0; i < no_rows; i++) {
		CU_ASSERT(table_insert_row(table, row_before, row_before_size));
		CU_ASSERT(check_row(table, i, &header_used, row_before));
	}
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	strcpy(column.name, "new_column");
	column.type = INTEGER;
	column.precision = sizeof(vp_data2_after[3]);
	CU_ASSERT(table_add_column(table, &column));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, (no_rows - (DATABLOCK_PAGE_SIZE / row_after_size)) * row_after_size);
	// make sure we got the right number of
	// datablocks after the data rearrange process
	CU_ASSERT_EQUAL(count_datablocks(table), 2);

	for (size_t i = 0; i < no_rows; i++) {
		/*
		 * the block is zeroed-out so we can anticipate which value/gap will be left
		 * when column is added. Additionally, we must ensure that the same number
		 * of rows exists in the table (though in this case the data will be spread
		 * across 2 datablocks instead of 1)
		 */
		CU_ASSERT(check_row(table, i, &header_used, row_after));
	}

	CU_ASSERT(table_insert_row(table, row_after, row_after_size));
	CU_ASSERT(check_row(table, no_rows, &header_used, row_after));
	CU_ASSERT(check_row_flags(table, no_rows + 1, &header_empty));

	CU_ASSERT_EQUAL(count_datablocks(table), 2);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset,
			(no_rows + 1 - (DATABLOCK_PAGE_SIZE / row_after_size)) * row_after_size);

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
