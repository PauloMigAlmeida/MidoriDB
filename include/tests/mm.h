#ifndef TESTS_MM_H
#define TESTS_MM_H

#include "tests/unittest.h"

/* test suites */
bool mm_init_suite(void);

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


#endif /* TESTS_MM_H */
