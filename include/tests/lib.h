#ifndef TESTS_LIB_H
#define TESTS_LIB_H

#include "tests/unittest.h"

/* test suites */
bool lib_init_suite(void);

/* unit tests */
void test_bit_test(void);
void test_bit_set(void);
void test_bit_clear(void);
void test_bit_toggle(void);

void test_strstarts(void);
void test_strrand(void);

void test_regex_ext_match_grp(void);

#endif /* TESTS_LIB_H */
