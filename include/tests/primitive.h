#ifndef TESTS_MM_H
#define TESTS_MM_H

#include <primitive/table.h>
#include <primitive/column.h>
#include <primitive/row.h>
#include <tests/unittest.h>

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
void test_table_vacuum(void);

/* utility functions used across primitive test suites */
void create_test_table_fixed_precision_columns(struct table **out, size_t column_count);
void create_test_table_var_precision_columns(struct table **out, size_t column_precision, size_t column_count);
void create_test_table_mixed_precision_columns(struct table **out, size_t column_precision, size_t column_count);

#endif /* TESTS_MM_H */
