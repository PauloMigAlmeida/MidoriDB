#ifndef TESTS_DATASTRUCTURE_H
#define TESTS_DATASTRUCTURE_H

#include "tests/unittest.h"

/* test suites */
bool datastructure_init_suite(void);

/* unit tests */
void test_initialise_linkedlist(void);
void test_linkedlist_add(void);
void test_linkedlist_del(void);

#endif /* TESTS_DATASTRUCTURE_H */
