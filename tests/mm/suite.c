#include "tests/mm.h"

bool mm_init_suite(void)
{
	CU_pSuite suite = NULL;

	ADD_SUITE(suite, "mm");
	/* data block */
	ADD_UNITTEST(suite, test_datablock_init);
	ADD_UNITTEST(suite, test_datablock_alloc);
	ADD_UNITTEST(suite, test_datablock_free);
	ADD_UNITTEST(suite, test_datablock_iterate);
	/* table */
	ADD_UNITTEST(suite, test_table_init);
	ADD_UNITTEST(suite, test_table_destroy);
	ADD_UNITTEST(suite, test_table_add_column);
	ADD_UNITTEST(suite, test_table_rem_column);
	ADD_UNITTEST(suite, test_table_insert_row);
	ADD_UNITTEST(suite, test_table_delete_row);
	ADD_UNITTEST(suite, test_table_update_row);

	return false;
}

