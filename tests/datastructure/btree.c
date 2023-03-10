#include "tests/datastructure.h"
#include "datastructure/btree.h"

void test_btree_init(void)
{
	struct btree_head *head;
	head = btree_init(100, &btree_cmp_ul);
	CU_ASSERT_PTR_NOT_NULL(head);
	btree_destroy(&head);
	CU_ASSERT_PTR_NULL(head);
}

void test_btree_destroy(void)
{
	/* empty */
	struct btree_head *head;
	head = btree_init(2, &btree_cmp_ul);
	CU_ASSERT_PTR_NOT_NULL(head);
	btree_destroy(&head);
	CU_ASSERT_PTR_NULL(head);

	/*
	 * stressing the splitting operations to ensure pointers are neither
	 * forgotten nor double freed
	 */
	uint64_t arr[10000];
	uint64_t val = 0xB1EE5;

	head = btree_init(2, &btree_cmp_ul);
	for (uint64_t i = 0; i < ARR_SIZE(arr); i++) {
		arr[i] = i;
		CU_ASSERT_EQUAL_FATAL(btree_insert(head, &arr[i], &val), 0);
	}

	btree_destroy(&head);
	CU_ASSERT_PTR_NULL(head);

}

void test_btree_lookup(void)
{
	struct btree_head *head;
	uint64_t keys[100];
	uint64_t values[100];

	head = btree_init(2, &btree_cmp_ul);
	CU_ASSERT_PTR_NOT_NULL(head);

	for (uint64_t i = 0; i < ARR_SIZE(keys); i++) {
		keys[i] = i;
		values[i] = 0xBEEF + i;
		CU_ASSERT_EQUAL_FATAL(btree_insert(head, &keys[i], &values[i]), 0);
	}

	for (uint64_t i = 0; i < ARR_SIZE(keys); i++) {
		CU_ASSERT_PTR_EQUAL(btree_lookup(head, &keys[i]), &values[i]);

	}

	// try to find an item that doesn't exist
	uint64_t bogus = 0xFFFFFFFFFF;
	CU_ASSERT_PTR_EQUAL(btree_lookup(head, &bogus), NULL);

	btree_destroy(&head);
	CU_ASSERT_PTR_NULL(head);
}

void test_btree_insert__before_split(void)
{
	struct btree_head *head;
	uint64_t arr[] = {1, 2, 3};
	uint64_t val = 0xDEADBEEF;

	head = btree_init(2, &btree_cmp_ul);
	CU_ASSERT_PTR_NOT_NULL(head);

	for (uint64_t i = 0; i < ARR_SIZE(arr); i++) {
		CU_ASSERT_EQUAL_FATAL(btree_insert(head, &arr[i], &val), 0);
	}

	CU_ASSERT_PTR_NOT_EQUAL(head->root, NULL);
	CU_ASSERT_PTR_NOT_EQUAL(head->root->keys, NULL);
	CU_ASSERT_EQUAL(head->root->key_count, 3);
	CU_ASSERT_PTR_EQUAL(head->root->keys[0].key, &arr[0]);
	CU_ASSERT_PTR_EQUAL(head->root->keys[0].value, &val);
	CU_ASSERT_PTR_EQUAL(head->root->keys[1].key, &arr[1]);
	CU_ASSERT_PTR_EQUAL(head->root->keys[1].value, &val);
	CU_ASSERT_PTR_EQUAL(head->root->keys[2].key, &arr[2]);
	CU_ASSERT_PTR_EQUAL(head->root->keys[2].value, &val);

	btree_traverse(head);
	btree_destroy(&head);
	CU_ASSERT_PTR_NULL(head);
}

void test_btree_insert__root_split(void)
{
	struct btree_head *head;
	struct btree_node *tmp_node;
	uint64_t arr[] = {1, 2, 3, 4, 5};
	uint64_t val = 0xDEADBEEF;

	head = btree_init(2, &btree_cmp_ul);
	CU_ASSERT_PTR_NOT_NULL(head);

	for (uint64_t i = 0; i < ARR_SIZE(arr); i++) {
		CU_ASSERT_EQUAL_FATAL(btree_insert(head, &arr[i], &val), 0);
	}

	CU_ASSERT_PTR_NOT_EQUAL(head->root, NULL);
	CU_ASSERT_PTR_NOT_EQUAL(head->root->keys, NULL);
	CU_ASSERT_EQUAL(head->root->key_count, 1);
	CU_ASSERT_PTR_EQUAL(head->root->keys[0].key, &arr[1]);
	CU_ASSERT_PTR_EQUAL(head->root->keys[0].value, &val);

	tmp_node = &head->root->children[0];
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].key, &arr[0]);
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].value, &val);

	tmp_node = &head->root->children[1];
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].key, &arr[2]);
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].value, &val);
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[1].key, &arr[3]);
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[1].value, &val);
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[2].key, &arr[4]);
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[2].value, &val);

	btree_traverse(head);
	btree_destroy(&head);
	CU_ASSERT_PTR_NULL(head);
}

void test_btree_insert__intermidiate_split(void)
{
	struct btree_head *head;
	struct btree_node *tmp_node;
	uint64_t arr[] = {1, 2, 3, 4, 5, 6};
	uint64_t val = 0xDEADBEEF;

	head = btree_init(2, &btree_cmp_ul);
	CU_ASSERT_PTR_NOT_NULL(head);

	for (uint64_t i = 0; i < ARR_SIZE(arr); i++) {
		CU_ASSERT_EQUAL_FATAL(btree_insert(head, &arr[i], &val), 0);
	}

	CU_ASSERT_PTR_NOT_EQUAL(head->root, NULL);
	CU_ASSERT_PTR_NOT_EQUAL(head->root->keys, NULL);
	CU_ASSERT_EQUAL(head->root->key_count, 2);
	CU_ASSERT_PTR_EQUAL(head->root->keys[0].key, &arr[1]);
	CU_ASSERT_PTR_EQUAL(head->root->keys[0].value, &val);
	CU_ASSERT_PTR_EQUAL(head->root->keys[1].key, &arr[3]);
	CU_ASSERT_PTR_EQUAL(head->root->keys[1].value, &val);

	tmp_node = &head->root->children[0];
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].key, &arr[0]);
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].value, &val);

	tmp_node = &head->root->children[1];
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].key, &arr[2]);
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].value, &val);

	tmp_node = &head->root->children[2];
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].key, &arr[4]);
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].value, &val);
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[1].key, &arr[5]);
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[1].value, &val);
	btree_traverse(head);
	btree_destroy(&head);
	CU_ASSERT_PTR_NULL(head);
}

void test_btree_insert__increase_height(void)
{
	struct btree_head *head;
	struct btree_node *tmp_node;
	uint64_t arr[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	uint64_t val = 0xDEADBEEF;

	head = btree_init(2, &btree_cmp_ul);
	CU_ASSERT_PTR_NOT_NULL(head);

	for (uint64_t i = 0; i < ARR_SIZE(arr); i++) {
		CU_ASSERT_EQUAL_FATAL(btree_insert(head, &arr[i], &val), 0);
	}

	CU_ASSERT_PTR_NOT_EQUAL(head->root, NULL);
	CU_ASSERT_PTR_NOT_EQUAL(head->root->keys, NULL);
	CU_ASSERT_EQUAL(head->root->key_count, 1);
	CU_ASSERT_PTR_EQUAL(head->root->keys[0].key, &arr[3]);
	CU_ASSERT_PTR_EQUAL(head->root->keys[0].value, &val);

	tmp_node = &head->root->children[0];
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].key, &arr[1]);
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].value, &val);

	tmp_node = &head->root->children[0].children[0];
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].key, &arr[0]);
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].value, &val);

	tmp_node = &head->root->children[0].children[1];
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].key, &arr[2]);
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].value, &val);

	tmp_node = &head->root->children[1];
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].key, &arr[5]);
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].value, &val);
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[1].key, &arr[7]);
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[1].value, &val);

	tmp_node = &head->root->children[1].children[0];
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].key, &arr[4]);
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].value, &val);

	tmp_node = &head->root->children[1].children[1];
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].key, &arr[6]);
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].value, &val);

	tmp_node = &head->root->children[1].children[2];
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].key, &arr[8]);
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[0].value, &val);
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[1].key, &arr[9]);
	CU_ASSERT_PTR_EQUAL(tmp_node->keys[1].value, &val);

	btree_traverse(head);
	btree_destroy(&head);
	CU_ASSERT_PTR_NULL(head);
}

void test_btree_update(void)
{
	struct btree_head *head;
	uint64_t keys[10];
	uint64_t initial_values[10];
	uint64_t new_values[10];

	head = btree_init(2, &btree_cmp_ul);
	CU_ASSERT_PTR_NOT_NULL(head);

	/* insert initial values */
	for (uint64_t i = 0; i < ARR_SIZE(keys); i++) {
		keys[i] = i;
		initial_values[i] = 0xBEEF + i;
		CU_ASSERT_EQUAL_FATAL(btree_insert(head, &keys[i], &initial_values[i]), 0);
		CU_ASSERT_PTR_EQUAL(btree_lookup(head, &keys[i]), &initial_values[i]);
	}

	/* update values */
	for (uint64_t i = 0; i < ARR_SIZE(keys); i++) {
		new_values[i] = 0xDEAD + i;
		CU_ASSERT_TRUE(btree_update(head, &keys[i], &new_values[i]))
		CU_ASSERT_PTR_EQUAL(btree_lookup(head, &keys[i]), &new_values[i]);
	}

	btree_traverse(head);
	btree_destroy(&head);
	CU_ASSERT_PTR_NULL(head);
}

void test_btree_remove(void)
{
	struct btree_head *head;
	uint64_t arr[10000];
	uint64_t val = 0xB1EE5;

	head = btree_init(2, &btree_cmp_ul);
	CU_ASSERT_PTR_NOT_NULL(head);

	for (uint64_t i = 0; i < ARR_SIZE(arr); i++) {
		arr[i] = i + 1;
		CU_ASSERT_EQUAL_FATAL(btree_insert(head, &arr[i], &val), 0);
	}

	for (uint64_t i = 0; i < ARR_SIZE(arr) / 2; i++) {
		CU_ASSERT_EQUAL_FATAL(btree_remove(head, &arr[i]), 0);
		CU_ASSERT_EQUAL_FATAL(btree_remove(head, &arr[ARR_SIZE(arr) - 1 - i]), 0);

	}

	btree_destroy(&head);
	CU_ASSERT_PTR_NULL(head);
}

