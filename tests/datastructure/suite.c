#include "tests/datastructure.h"

bool datastructure_init_suite(void) {
	CU_pSuite suite = NULL;

	ADD_SUITE(suite, "datastructure");
	/* linkedlist */
	ADD_UNITTEST(suite, test_initialise_linkedlist);
	ADD_UNITTEST(suite, test_linkedlist_add);
	ADD_UNITTEST(suite, test_linkedlist_del);
	ADD_UNITTEST(suite, test_linkedlist_iterate);
	/* btree */
	ADD_UNITTEST(suite, test_btree_init);
	ADD_UNITTEST(suite, test_btree_destroy);
	ADD_UNITTEST(suite, test_btree_lookup);
	ADD_UNITTEST(suite, test_btree_update);
	ADD_UNITTEST(suite, test_btree_remove);

	return false;
}


