#include "tests/lib.h"

bool lib_init_suite(void)
{
	CU_pSuite suite = NULL;

	ADD_SUITE(suite, "lib");
	/* bit */
	ADD_UNITTEST(suite, test_bit_test);
	ADD_UNITTEST(suite, test_bit_set);
	ADD_UNITTEST(suite, test_bit_clear);
	ADD_UNITTEST(suite, test_bit_toggle);

	return false;
}

