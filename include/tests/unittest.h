#ifndef TESTS_UNITTEST_H
#define TESTS_UNITTEST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <CUnit/Basic.h>
#include <tests/utils.h>

#define ADD_SUITE(suite, name)				\
	do {						\
		suite = CU_add_suite(name, NULL, NULL);	\
		if (!suite) {				\
			CU_cleanup_registry();		\
			return true;			\
	        }					\
	}while (0);

#define ADD_SUITE_WITH_SETUP_TEARDOWN(suite, name, setup, teardown) 					\
	do {												\
		suite = CU_add_suite_with_setup_and_teardown(name, NULL, NULL, setup, teardown);	\
		if (!suite) {										\
			CU_cleanup_registry();								\
			return true;									\
	        }											\
	}while (0);

#define ADD_UNITTEST(suite, fnc)			\
	if (!CU_add_test(suite, #fnc, fnc)) {		\
		CU_cleanup_registry();			\
		return true;				\
	}

#endif /* TESTS_UNITTEST_H */
