#ifndef TESTS_UNITTEST_H
#define TESTS_UNITTEST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "CUnit/Basic.h"


#define ADD_SUITE(suite, name)				\
	do {						\
		suite = CU_add_suite(name,		\
				NULL,			\
				NULL);			\
	        if (!suite) {				\
		        CU_cleanup_registry();		\
			return true;			\
	        }					\
	}while (0);

#define ADD_UNITTEST(suite, fnc)		\
	if (!CU_add_test(suite, #fnc, fnc)) {	\
		CU_cleanup_registry();		\
		return true;			\
	}

#endif /* TESTS_UNITTEST_H */
