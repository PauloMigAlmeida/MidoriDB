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
	/* string */
	ADD_UNITTEST(suite, test_strstarts);
	ADD_UNITTEST(suite, test_strrand);
	/* regex */
	ADD_UNITTEST(suite, test_regex_ext_match_grp);

	return false;
}

