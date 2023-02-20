#ifndef TESTS_DATASTRUCTURE_H
#define TESTS_DATASTRUCTURE_H

#include "tests/unittest.h"

/* test suites */
bool datastructure_init_suite(void);

/* unit tests */
void test_initialise_linkedlist(void);
void test_linkedlist_add(void);
void test_linkedlist_del(void);
void test_linkedlist_iterate(void);
void test_btree_init(void);
void test_btree_destroy(void);
void test_btree_lookup(void);
void test_btree_insert(void);
void test_btree_update(void);
void test_btree_remove(void);

#endif /* TESTS_DATASTRUCTURE_H */
