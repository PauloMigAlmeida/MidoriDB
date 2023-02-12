#include "tests/datastructure.h"

bool datastructure_init_suite(void) {
	CU_pSuite suite = NULL;

	ADD_SUITE(suite, "datastructure");
	ADD_UNITTEST(suite, test_initialise_linkedlist);
	
	return false;
}


