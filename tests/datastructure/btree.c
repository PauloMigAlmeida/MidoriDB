#include "tests/datastructure.h"
#include "datastructure/btree.h"

static void btree_traverse(struct btree_head *head, struct btree_node *node, int level)
{
	for (int j = 0; j < level; j++)
		printf("\t");
	printf("[");
	for (int i = 0; i < node->key_count; i++) {
		printf("%lu", *(uint64_t*)node->keys[i].key);
		if (i != node->key_count - 1)
			printf(", ");
	}
	printf("] - {keys: keys: %p, children: %p}\n", (void*)node->keys, (void*)node->children);

	if (!node->is_leaf)
		for (int i = 0; i <= node->key_count; i++)
			btree_traverse(head, &node->children[i], level + 1);
}

void test_btree_init(void)
{
	CU_ASSERT(true);
}

void test_btree_destroy(void)
{
	/* empty */
	struct btree_head *head;
	head = btree_init(2, sizeof(uint64_t), sizeof(uint64_t), &btree_cmp_ul);
	btree_destroy(&head);
	CU_ASSERT(head == NULL);

	/*
	 * stressing the splitting operations to ensure pointers are neither
	 * forgotten nor double freed
	 */
	uint64_t arr[10000];
	uint64_t val = 0xB1EE5;

	head = btree_init(2, sizeof(uint64_t), sizeof(uint64_t), &btree_cmp_ul);
	for (uint64_t i = 0; i < ARR_SIZE(arr); i++) {
		arr[i] = i;
		btree_insert(head, arr + i, &val);
	}

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

	btree_traverse(head, head->root, 0);

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
	uint64_t key5 = 5;
	uint64_t val = 0xDEADBEEF;

	head = btree_init(2, sizeof(uint64_t), sizeof(uint64_t), &btree_cmp_ul);

	btree_insert(head, &key1, &val);
	btree_insert(head, &key2, &val);
	btree_insert(head, &key3, &val);
	btree_insert(head, &key4, &val);
	btree_insert(head, &key5, &val);

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
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[2].key), key5);
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[2].value), val);

	btree_traverse(head, head->root, 0);

	btree_destroy(&head);
}

void test_btree_insert__intermidiate_split(void)
{
	struct btree_head *head;
	struct btree_node *tmp_node;
	uint64_t key1 = 1;
	uint64_t key2 = 2;
	uint64_t key3 = 3;
	uint64_t key4 = 4;
	uint64_t key5 = 5;
	uint64_t key6 = 6;
	uint64_t val = 0xDEADBEEF;

	head = btree_init(2, sizeof(uint64_t), sizeof(uint64_t), &btree_cmp_ul);

	btree_insert(head, &key1, &val);
	btree_insert(head, &key2, &val);
	btree_insert(head, &key3, &val);
	btree_insert(head, &key4, &val);
	btree_insert(head, &key5, &val);
	btree_insert(head, &key6, &val);

	CU_ASSERT(head->root != NULL);
	CU_ASSERT(head->root->keys != NULL);
	CU_ASSERT_EQUAL(head->root->key_count, 2);
	CU_ASSERT_EQUAL(*((uint64_t* )head->root->keys[0].key), key2);
	CU_ASSERT_EQUAL(*((uint64_t* )head->root->keys[0].value), val);
	CU_ASSERT_EQUAL(*((uint64_t* )head->root->keys[1].key), key4);
	CU_ASSERT_EQUAL(*((uint64_t* )head->root->keys[1].value), val);

	tmp_node = &head->root->children[0];
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].key), key1);
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].value), val);

	tmp_node = &head->root->children[1];
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].key), key3);
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].value), val);

	tmp_node = &head->root->children[2];
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].key), key5);
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].value), val);
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[1].key), key6);
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[1].value), val);
	btree_traverse(head, head->root, 0);

	btree_destroy(&head);
}

void test_btree_insert__increase_height(void)
{
	struct btree_head *head;
	struct btree_node *tmp_node;
	uint64_t key1 = 1;
	uint64_t key2 = 2;
	uint64_t key3 = 3;
	uint64_t key4 = 4;
	uint64_t key5 = 5;
	uint64_t key6 = 6;
	uint64_t key7 = 7;
	uint64_t key8 = 8;
	uint64_t key9 = 9;
	uint64_t key10 = 10;
	uint64_t val = 0xDEADBEEF;

	head = btree_init(2, sizeof(uint64_t), sizeof(uint64_t), &btree_cmp_ul);

	btree_insert(head, &key1, &val);
	btree_insert(head, &key2, &val);
	btree_insert(head, &key3, &val);
	btree_insert(head, &key4, &val);
	btree_insert(head, &key5, &val);
	btree_insert(head, &key6, &val);
	btree_insert(head, &key7, &val);
	btree_insert(head, &key8, &val);
	btree_insert(head, &key9, &val);
	btree_insert(head, &key10, &val);

	CU_ASSERT(head->root != NULL);
	CU_ASSERT(head->root->keys != NULL);
	CU_ASSERT_EQUAL(head->root->key_count, 1);
	CU_ASSERT_EQUAL(*((uint64_t* )head->root->keys[0].key), key4);
	CU_ASSERT_EQUAL(*((uint64_t* )head->root->keys[0].value), val);

	tmp_node = &head->root->children[0];
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].key), key2);
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].value), val);

	tmp_node = &head->root->children[0].children[0];
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].key), key1);
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].value), val);

	tmp_node = &head->root->children[0].children[1];
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].key), key3);
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].value), val);

	tmp_node = &head->root->children[1];
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].key), key6);
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].value), val);
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[1].key), key8);
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[1].value), val);

	tmp_node = &head->root->children[1].children[0];
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].key), key5);
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].value), val);

	tmp_node = &head->root->children[1].children[1];
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].key), key7);
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].value), val);

	tmp_node = &head->root->children[1].children[2];
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].key), key9);
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[0].value), val);
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[1].key), key10);
	CU_ASSERT_EQUAL(*((uint64_t* )tmp_node->keys[1].value), val);

	btree_traverse(head, head->root, 0);

	btree_destroy(&head);
}

void test_btree_update(void)
{
	CU_ASSERT(true);
}

static void btree_traverse_pointers(struct btree_head *head, struct btree_node *node)
{
//	printf("%p\n", (void*)node);
	printf("%p\n", (void*)node->keys);
	printf("%p\n", (void*)node->children);

	if (!node->is_leaf)
		for (int i = 0; i <= node->key_count; i++)
			btree_traverse_pointers(head, &node->children[i]);
}

void test_btree_remove(void)
{
	struct btree_head *head;
	uint64_t arr[4105];
	uint64_t val = 0xB1EE5;

	head = btree_init(2, sizeof(uint64_t), sizeof(uint64_t), &btree_cmp_ul);

	for (uint64_t i = 0; i < ARR_SIZE(arr); i++) {
		arr[i] = i + 1;
		btree_insert(head, &arr[i], &val);
	}

	printf("\n\n");
//	btree_traverse(head, head->root, 0);
//	btree_traverse_pointers(head, head->root);
//	printf("\n");
//
//	printf("btree_destroy\n\n");

	btree_remove(head, &arr[0]);

//	btree_traverse(head, head->root, 0);
//	btree_traverse_pointers(head, head->root);

	btree_destroy(&head);
	CU_ASSERT(head == NULL);
}

