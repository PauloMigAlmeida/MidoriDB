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

void test_database_open(void);
void test_database_close(void);
void test_database_add_table(void);

#endif /* INCLUDE_TESTS_ENGINE_H_ */