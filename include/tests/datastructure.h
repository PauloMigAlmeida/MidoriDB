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
void test_btree_insert__before_split(void);
void test_btree_insert__root_split(void);
void test_btree_insert__intermidiate_split(void);
void test_btree_insert__increase_height(void);
void test_btree_update(void);
void test_btree_remove(void);

void test_vector_init(void);
void test_vector_push(void);
void test_vector_free(void);

void test_stack_init(void);
void test_stack_push(void);
void test_stack_pop(void);
void test_stack_free(void);
void test_stack_peek(void);
void test_stack_peek_pos(void);

#endif /* TESTS_DATASTRUCTURE_H */
