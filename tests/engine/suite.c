#include "tests/engine.h"

void engine_setup(void)
{
	printf("setup called\n");
}

void engine_teardown(void)
{
	printf("teardown called\n");
}

bool engine_init_suite(void)
{
	CU_pSuite suite = NULL;

	ADD_SUITE_WITH_SETUP_TEARDOWN(suite, "engine", &engine_setup, &engine_teardown);
	if (!suite) {
		CU_cleanup_registry();
		return true;
	}

	/* executor */
	ADD_UNITTEST(suite, test_executor_run);

	return false;
}

