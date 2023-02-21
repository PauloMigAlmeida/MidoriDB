#include "tests/datastructure.h"
#include "datastructure/btree.h"

void test_btree_init(void) {
	CU_ASSERT(true);
}

void test_btree_destroy(void) {
	struct btree_head* head;
	head = btree_init(2, sizeof(uint64_t), sizeof(uint64_t), &btree_cmp_ul);
	btree_destroy(&head);
	CU_ASSERT(head == NULL);
}

void test_btree_lookup(void) {
	struct btree_head* head;
	head = btree_init(3, sizeof(uint64_t), sizeof(uint64_t), &btree_cmp_ul);


	CU_ASSERT(true);
	btree_destroy(&head);
	CU_ASSERT(head == NULL);
}

void test_btree_insert(void) {
	struct btree_head* head;
	uint64_t key = 10;
	uint64_t val = 0xDEADBEEF;

	head = btree_init(3, sizeof(uint64_t), sizeof(uint64_t), &btree_cmp_ul);

	/* insert first entry of root node */
	btree_insert(head, &key, &val);
	CU_ASSERT(head->root != NULL);
	CU_ASSERT(head->root->keys != NULL);
	CU_ASSERT_EQUAL(head->root->key_count, 1);
	CU_ASSERT_EQUAL(*((uint64_t*)head->root->keys[0].key), key);
	CU_ASSERT_EQUAL(*((uint64_t*)head->root->keys[0].value), val);

	btree_destroy(&head);
}

void test_btree_update(void) {
	CU_ASSERT(true);
}

void test_btree_remove(void) {
	CU_ASSERT(true);
}

