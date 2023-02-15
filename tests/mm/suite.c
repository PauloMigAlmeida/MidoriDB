#include "tests/mm.h"

bool mm_init_suite(void) {
	CU_pSuite suite = NULL;

	ADD_SUITE(suite, "mm");
	ADD_UNITTEST(suite, test_datablock_init);
	ADD_UNITTEST(suite, test_datablock_alloc);
	ADD_UNITTEST(suite, test_datablock_free);
	ADD_UNITTEST(suite, test_datablock_iterate);
	
	return false;
}


