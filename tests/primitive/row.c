/*
 * table.c
 *
 *  Created on: 7/03/2023
 *      Author: paulo
 */

#include <primitive/table.h>
#include <tests/primitive.h>

static struct row_header_flags header_empty = {.deleted = false, .empty = true};
static struct row_header_flags header_deleted = {.deleted = true, .empty = false};
static struct row_header_flags header_used = {.deleted = false, .empty = false};

void test_table_insert_row(void)
{
	struct table *table;

	int fp_data[] = {0, 1, 2, 3, 0};
	struct row *row = build_row(fp_data, sizeof(fp_data), NULL, 0);

	int fp_null_fields[] = {0, 3};
	struct row *row_nulls = build_row(fp_data, sizeof(fp_data), fp_null_fields, ARR_SIZE(fp_null_fields));

	size_t row_size = struct_size(row, data, sizeof(fp_data));

	/* valid case - fixed precision columns - empty table */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(fp_data));
	CU_ASSERT(table_destroy(&table));

	/* valid case - fixed precision columns - normal case */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(fp_data));
	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_row(table, 0, &header_used, row));
	CU_ASSERT(check_row(table, 1, &header_used, row));
	CU_ASSERT(check_row_flags(table, 2, &header_empty));
	CU_ASSERT(table_destroy(&table));

	/* valid case - fixed precision columns - check if free_dtbkl_offset is moving as expected */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(fp_data));
	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_row(table, 0, &header_used, row));
	CU_ASSERT(check_row(table, 1, &header_used, row));
	CU_ASSERT(check_row(table, 2, &header_used, row));
	CU_ASSERT(check_row_flags(table, 3, &header_empty));

	CU_ASSERT(table_delete_row(table, fetch_datablock(table, 0), row_size));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(check_row(table, 0, &header_used, row));
	CU_ASSERT(check_row(table, 1, &header_deleted, row));
	CU_ASSERT(check_row(table, 2, &header_used, row));
	CU_ASSERT(check_row_flags(table, 3, &header_empty));

	CU_ASSERT(table_destroy(&table));

	/* valid case - fixed precision columns - force allocate new data blocks */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(fp_data));

	for (size_t i = 0; i < (DATABLOCK_PAGE_SIZE / row_size) * 3; i++) {
		CU_ASSERT(table_insert_row(table, row, row_size));
		CU_ASSERT(check_row(table, i, &header_used, row));
	}
	CU_ASSERT_EQUAL(count_datablocks(table), 3);
	CU_ASSERT(table_destroy(&table));

	/* valid case - fixed precision columns - insert row with null fields */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(fp_data));
	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT(table_insert_row(table, row_nulls, row_size));
	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_row(table, 0, &header_used, row));
	CU_ASSERT(check_row(table, 1, &header_used, row_nulls));
	CU_ASSERT(check_row(table, 2, &header_used, row));
	CU_ASSERT(check_row_flags(table, 3, &header_empty));
	CU_ASSERT(table_destroy(&table));

	/* invalid case - fixed precision columns - different row size */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(fp_data));
	CU_ASSERT_FALSE(table_insert_row(table, row, row_size + 1));
	CU_ASSERT_EQUAL(count_datablocks(table), 0);
	CU_ASSERT(table_destroy(&table));

	/* invalid case - fixed precision columns - invalid row size */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(fp_data));
	CU_ASSERT_FALSE(table_insert_row(table, row, 0));
	CU_ASSERT_EQUAL(count_datablocks(table), 0);
	CU_ASSERT(table_destroy(&table));

	free(row);
	free(row_nulls);

	/* valid case - columns with variable precision - common */
	char *vp_data_1 = "test1";
	char *vp_data_2 = "test2";
	char *vp_data_3 = "test3";
	uintptr_t vp_data[] = {(uintptr_t)vp_data_1, (uintptr_t)vp_data_2, (uintptr_t)vp_data_3};

	row = build_row(vp_data, sizeof(vp_data), NULL, 0);
	row_size = struct_size(row, data, sizeof(vp_data));

	create_test_table_var_precision_columns(&table, 6, ARR_SIZE(vp_data));
	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_row(table, 0, &header_used, row));
	CU_ASSERT(check_row(table, 1, &header_used, row));
	CU_ASSERT(check_row_flags(table, 2, &header_empty));
	CU_ASSERT(table_destroy(&table));

	free(row);

	/* valid case - columns with both fixed and variable precision - common */
	char *mp_data_1 = "test123";
	uintptr_t mp_data[] = {1, (uintptr_t)mp_data_1, 2};

	row = build_row(mp_data, sizeof(mp_data), NULL, 0);
	row_size = struct_size(row, data, sizeof(mp_data));

	create_test_table_mixed_precision_columns(&table, 8, ARR_SIZE(mp_data));
	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_row(table, 0, &header_used, row));
	CU_ASSERT(check_row(table, 1, &header_used, row));
	CU_ASSERT(check_row_flags(table, 2, &header_empty));
	CU_ASSERT(table_destroy(&table));

	free(row);
}

void test_table_delete_row(void)
{
	struct table *table;

	int fp_data[] = {0, 1, 2, 3, 0};
	int fp_null_fields[] = {0, 3};

	struct row *row = build_row(fp_data, sizeof(fp_data), NULL, 0);
	struct row *row_nulls = build_row(fp_data, sizeof(fp_data), fp_null_fields, ARR_SIZE(fp_null_fields));

	size_t row_size = struct_size(row, data, sizeof(fp_data));

	/* valid case - fixed precision columns - common */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(fp_data));

	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_row(table, 0, &header_used, row));
	CU_ASSERT(check_row_flags(table, 1, &header_empty));

	CU_ASSERT(table_delete_row(table, fetch_datablock(table, 0), 0));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(check_row(table, 0, &header_deleted, row));
	CU_ASSERT(check_row_flags(table, 1, &header_empty));
	CU_ASSERT(table_destroy(&table));

	/* valid case - fixed precision columns - row with null values */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(fp_data));

	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT(table_insert_row(table, row_nulls, row_size));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size * 2);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_row(table, 0, &header_used, row));
	CU_ASSERT(check_row(table, 1, &header_used, row_nulls));
	CU_ASSERT(check_row_flags(table, 2, &header_empty));

	CU_ASSERT(table_delete_row(table, fetch_datablock(table, 0), row_size));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size * 2);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(check_row(table, 0, &header_used, row));
	CU_ASSERT(check_row(table, 1, &header_deleted, row_nulls));
	CU_ASSERT(check_row_flags(table, 2, &header_empty));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size * 2);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(table_destroy(&table));

	/* invalid case - fixed precision columns - invalid offset  */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(fp_data));

	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size);
	CU_ASSERT(check_row(table, 0, &header_used, row));
	CU_ASSERT(check_row_flags(table, 1, &header_empty));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT_FALSE(table_delete_row(table, fetch_datablock(table, 0), DATABLOCK_PAGE_SIZE + 1));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size);
	CU_ASSERT(check_row(table, 0, &header_used, row));
	CU_ASSERT(check_row_flags(table, 1, &header_empty));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(table_destroy(&table));

	/* invalid case - fixed precision columns - invalid block  */
	create_test_table_fixed_precision_columns(&table, ARR_SIZE(fp_data));
	CU_ASSERT_FALSE(table_delete_row(table, NULL, 0));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 0);
	CU_ASSERT_EQUAL(count_datablocks(table), 0);

	CU_ASSERT(table_destroy(&table));

	free(row);
	free(row_nulls);

	/* valid case - columns with variable precision - common */
	char *vp_data_1 = "test1";
	char *vp_data_2 = "test2";
	char *vp_data_3 = "test3";
	uintptr_t vp_data[] = {(uintptr_t)vp_data_1, (uintptr_t)vp_data_2, (uintptr_t)vp_data_3};

	row = build_row(vp_data, sizeof(vp_data), NULL, 0);
	row_size = struct_size(row, data, sizeof(vp_data));

	create_test_table_var_precision_columns(&table, 6, ARR_SIZE(vp_data));
	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_row(table, 0, &header_used, row));
	CU_ASSERT(check_row_flags(table, 1, &header_empty));

	CU_ASSERT(table_delete_row(table, fetch_datablock(table, 0), 0));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(check_row(table, 0, &header_deleted, row));
	CU_ASSERT(check_row_flags(table, 1, &header_empty));
	CU_ASSERT(table_destroy(&table));

	free(row);

	/* valid case - columns with both fixed and variable precision - common */
	char *mp_data_1 = "test123";
	uintptr_t mp_data[] = {1, (uintptr_t)mp_data_1, 2};

	row = build_row(mp_data, sizeof(mp_data), NULL, 0);
	row_size = struct_size(row, data, sizeof(mp_data));

	create_test_table_mixed_precision_columns(&table, 8, ARR_SIZE(mp_data));
	CU_ASSERT(table_insert_row(table, row, row_size));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_row(table, 0, &header_used, row));
	CU_ASSERT(check_row_flags(table, 1, &header_empty));

	CU_ASSERT(table_delete_row(table, fetch_datablock(table, 0), 0));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(check_row(table, 0, &header_deleted, row));
	CU_ASSERT(check_row_flags(table, 1, &header_empty));
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
	CU_ASSERT(check_row_flags(table, 2, &header_empty));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, table_calc_row_size(table) * 2);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(table_update_row(table, fetch_datablock(table, 0), row_size, row_new, row_size));
	CU_ASSERT(check_row(table, 0, &header_used, row_old));
	CU_ASSERT(check_row(table, 1, &header_used, row_new));
	CU_ASSERT(check_row_flags(table, 2, &header_empty));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size * 2);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(table_destroy(&table));
	free(row_old);
	free(row_new);

	/* valid case - variable precision columns with null columns */
	char vpn_old_data_1[6] = {0};
	char *vpn_old_data_2 = "test2";
	char vpn_old_data_3[6] = {0};
	int vpn_old_null_fields[] = {0, 2};
	uintptr_t vpn_old_data[] = {(uintptr_t)vpn_old_data_1, (uintptr_t)vpn_old_data_2, (uintptr_t)vpn_old_data_3};

	char *vpn_new_data_1 = "test4";
	char vpn_new_data_2[6] = {0};
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
	CU_ASSERT(check_row_flags(table, 2, &header_empty));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, table_calc_row_size(table) * 2);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(table_update_row(table, fetch_datablock(table, 0), row_size, row_new, row_size));
	CU_ASSERT(check_row(table, 0, &header_used, row_old));
	CU_ASSERT(check_row(table, 1, &header_used, row_new));
	CU_ASSERT(check_row_flags(table, 2, &header_empty));
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, row_size * 2);
	CU_ASSERT_EQUAL(count_datablocks(table), 1);

	CU_ASSERT(table_destroy(&table));
	free(row_old);
	free(row_new);
}
