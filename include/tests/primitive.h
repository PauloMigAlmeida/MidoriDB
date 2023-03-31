#ifndef TESTS_MM_H
#define TESTS_MM_H

#include <primitive/table.h>
#include "tests/unittest.h"

/* test suites */
bool primitive_init_suite(void);

/* unit tests */
void test_datablock_init(void);
void test_datablock_alloc(void);
void test_datablock_free(void);
void test_datablock_iterate(void);

void test_table_init(void);
void test_table_destroy(void);
void test_table_add_column(void);
void test_table_rem_column(void);
void test_table_insert_row(void);
void test_table_delete_row(void);
void test_table_update_row(void);

/* utility functions used across mm test suites */
size_t count_datablocks(struct table *table);
struct row* fetch_row(struct table *table, size_t row_num);
bool check_row_data(struct table *table, size_t row_num, void *expected);
bool check_row_flags(struct table *table, size_t row_num, struct row_header_flags *expected);
bool check_row(struct table *table, size_t row_num, struct row_header_flags *exp_header_flags, struct row *row);
struct datablock* fetch_datablock(struct table *table, size_t idx);
struct row* build_row(void *data, size_t data_len, int *null_cols_idx, size_t cols_idx_len);
void create_test_table_fixed_precision_columns(struct table **out, size_t column_count);
void create_test_table_var_precision_columns(struct table **out, size_t column_precision, size_t column_count);
void create_test_table_mixed_precision_columns(struct table **out, size_t column_precision, size_t column_count);

#endif /* TESTS_MM_H */
