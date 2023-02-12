#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "CUnit/Basic.h"
#include "datastructure/linkedlist.h"

extern void test_initialise_linkedlist(void);

int init_suite_success(void) { return 0; }
int clean_suite_success(void) { return 0; }

bool datastructure_init_suite(void) {
	CU_pSuite pSuite = NULL;

	/* add a suite to the registry */
	pSuite = CU_add_suite("Suite_success", NULL, NULL);
	if (!pSuite) {
		CU_cleanup_registry();
		return true;
	}

	/* add the tests to the suite */
	if (!CU_add_test(pSuite, "test_initialise_linkedlist", test_initialise_linkedlist)) {
		CU_cleanup_registry();
		return true;
	}
	return false;
}


