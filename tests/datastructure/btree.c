#include "tests/datastructure.h"
#include "datastructure/btree.h"

//static void btree_traverse(struct btree_head *head, struct btree_node *node)
//{
//	for (int i = 0; i < node->key_count; i++) {
//		if (!node->is_leaf)
//			btree_traverse(head, &node->children[i]);
//
//		printf("key_%d: %lu\n", i, *(uint64_t*)node->keys[i].key);
//	}
//
//	if (!node->is_leaf)
//		btree_traverse(head, &node->children[node->key_count]);
//}

void test_btree_init(void)
{
	CU_ASSERT(true);
}

void test_btree_destroy(void)
{
	struct btree_head *head;
	head = btree_init(2, sizeof(uint64_t), sizeof(uint64_t), &btree_cmp_ul);
	btree_destroy(&head);
	CU_ASSERT(head == NULL);
}

void test_btree_lookup(void)
{
	struct btree_head *head;
	head = btree_init(3, sizeof(uint64_t), sizeof(uint64_t), &btree_cmp_ul);

	CU_ASSERT(true);
	btree_destroy(&head);
	CU_ASSERT(head == NULL);
}

void test_btree_insert__before_split(void)
{
	struct btree_head *head;
	uint64_t key1 = 1;
	uint64_t key2 = 2;
	uint64_t key3 = 3;
	uint64_t val = 0xDEADBEEF;

	head = btree_init(2, sizeof(uint64_t), sizeof(uint64_t), &btree_cmp_ul);

	btree_insert(head, &key1, &val);
	btree_insert(head, &key2, &val);
	btree_insert(head, &key3, &val);

	CU_ASSERT(head->root != NULL);
	CU_ASSERT(head->root->keys != NULL);
	CU_ASSERT_EQUAL(head->root->key_count, 3);
	CU_ASSERT_EQUAL(*((uint64_t* )head->root->keys[0].key), key1);
	CU_ASSERT_EQUAL(*((uint64_t* )head->root->keys[0].value), val);
	CU_ASSERT_EQUAL(*((uint64_t* )head->root->keys[1].key), key2);
	CU_ASSERT_EQUAL(*((uint64_t* )head->root->keys[1].value), val);
	CU_ASSERT_EQUAL(*((uint64_t* )head->root->keys[2].key), key3);
	CU_ASSERT_EQUAL(*((uint64_t* )head->root->keys[2].value), val);

	btree_destroy(&head);
}

void test_btree_insert__root_split(void)
{
	struct btree_head *head;
	struct btree_node *tmp_node;
	uint64_t key1 = 1;
	uint64_t key2 = 2;
	uint64_t key3 = 3;
	uint64_t key4 = 4;
	uint64_t val = 0xDEADBEEF;

	head = btree_init(2, sizeof(uint64_t), sizeof(uint64_t), &btree_cmp_ul);

	btree_insert(head, &key1, &val);
	btree_insert(head, &key2, &val);
	btree_insert(head, &key3, &val);
	btree_insert(head, &key4, &val);

//	printf("\n\n");
//	btree_traverse(head, head->root);

	CU_ASSERT(head->root != NULL);
	CU_ASSERT(head->root->keys != NULL);
	CU_ASSERT_EQUAL(head->root->key_count, 1);
	CU_ASSERT_EQUAL(*((uint64_t* )head->root->keys[0].key), key2);
	CU_ASSERT_EQUAL(*((uint64_t* )head->root->keys[0].value), val);

	tmp_node = &head->root->children[0];
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].key), key1);
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].value), val);

	tmp_node = &head->root->children[1];
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].key), key3);
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].value), val);
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[1].key), key4);
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[1].value), val);

	btree_destroy(&head);
}

void test_btree_update(void)
{
	CU_ASSERT(true);
}

void test_btree_remove(void)
{
	CU_ASSERT(true);
}

