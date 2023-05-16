#include <tests/datastructure.h>

bool datastructure_init_suite(void)
{
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
	ADD_UNITTEST(suite, test_btree_insert__before_split);
	ADD_UNITTEST(suite, test_btree_insert__root_split);
	ADD_UNITTEST(suite, test_btree_insert__intermidiate_split);
	ADD_UNITTEST(suite, test_btree_insert__increase_height);
	ADD_UNITTEST(suite, test_btree_update);
	ADD_UNITTEST(suite, test_btree_remove);
	/* vector */
	ADD_UNITTEST(suite, test_vector_init);
	ADD_UNITTEST(suite, test_vector_push);
	ADD_UNITTEST(suite, test_vector_free);
	/* stack */
	ADD_UNITTEST(suite, test_stack_init);
	ADD_UNITTEST(suite, test_stack_push);
	ADD_UNITTEST(suite, test_stack_pop);
	ADD_UNITTEST(suite, test_stack_free);
	ADD_UNITTEST(suite, test_stack_peek);
	ADD_UNITTEST(suite, test_stack_peek_pos);

	return false;
}

