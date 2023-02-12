#include "tests/unittest.h"
#include "tests/datastructure.h"

int main(void) {
	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	/* add suites & tests */
	if(datastructure_init_suite()){
		return CU_get_error();
	}

	/* Run all tests using the basic interface */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	printf("\n");
	CU_basic_show_failures(CU_get_failure_list());
	printf("\n\n");

	/* Clean up registry and return */
	CU_cleanup_registry();
	return CU_get_error();
}

