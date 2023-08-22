#include "tests/engine.h"

bool engine_init_suite(void)
{
	CU_pSuite suite = NULL;

	ADD_SUITE(suite, "engine");
	if (!suite) {
		CU_cleanup_registry();
		return true;
	}

	/* midoridb */
	ADD_UNITTEST(suite, test_database_open);
	ADD_UNITTEST(suite, test_database_close);
	ADD_UNITTEST(suite, test_database_add_table);
	ADD_UNITTEST(suite, test_database_table_exists);

	/* executor */
	ADD_UNITTEST(suite, test_optimiser_run);

	/* executor */
	ADD_UNITTEST(suite, test_executor_run);

	return false;
}

