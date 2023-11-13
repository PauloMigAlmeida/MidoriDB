/*
 * engine.h
 *
 *  Created on: 8/06/2023
 *      Author: paulo
 */

#ifndef INCLUDE_TESTS_ENGINE_H_
#define INCLUDE_TESTS_ENGINE_H_

#include <tests/unittest.h>

/* test suites */
bool engine_init_suite(void);

/* unit tests */
void test_executor_run(void);

void test_optimiser_run(void);

void test_database_open(void);
void test_database_close(void);
void test_database_add_table(void);
void test_database_table_exists(void);

/* sub tests */
void test_optimiser_insert(void);
void test_optimiser_select(void);

#endif /* INCLUDE_TESTS_ENGINE_H_ */
