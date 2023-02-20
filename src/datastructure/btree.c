#include "datastructure/btree.h"

int btree_cmp_str(void *key1, void *key2) {
	return strcmp(key1, key2);
}

int btree_cmp_ul(void *key1, void *key2) {
	uint64_t *k1, *k2;
	k1 = key1;
	k2 = key2;

	if (*k1 == *k2)
		return 0;
	else if (*k1 < *k2)
		return -1;
	else
		return 1;
}

struct btree_head *btree_init(int key_len,
			      size_t key_size,
			      size_t val_size,
			      int (*cmp_fn)(void *, void *)) {
	struct btree_head *head = NULL;

	BUG_ON(key_len < 3);
	BUG_ON(!cmp_fn);

	if ((head = malloc(sizeof(*head)))) {
		memset(head, 0, sizeof(*head));
		head->key_len = key_len;
		head->key_size = key_size;
		head->val_size = val_size;
		head->cmp_fn = cmp_fn;
	} else {
		die("couldn't alloc mem for btree");
	}

	return head;
}

void btree_destroy(struct btree_head *head) {

}

void *btree_lookup(struct btree_head *head, unsigned long *key) {
	return head;
}

bool btree_update(struct btree_head *head, unsigned long *key, void *val) {
	return true;
}

void btree_insert(struct btree_head *head, void *key, void *val) {
	struct btree_node *node;
	struct btree_node_tuple *keys;
	void *tuple_key, *tuple_val;

	if (!head->node) {

		node = malloc(sizeof(*node));
		keys = calloc(head->key_len, head->key_size);
		tuple_key = malloc(head->key_size);
		tuple_val = malloc(head->val_size);

		if (node && keys && tuple_key && tuple_val) {
			memset(node, 0, sizeof(*node));
			node->is_leaf = false;

			memcpy(tuple_key, key, head->key_size);
			memcpy(tuple_val, val, head->val_size);
			keys[0].key = tuple_key;
			keys[0].value = tuple_val;

			node->keys = keys;
			node->key_count = 1;
			head->node = node;
		} else {
			die("couldn't alloc node/key for btree");
		}

	} else {
	}
}

void *btree_remove(struct btree_head *head, unsigned long *key) {

	return head;
}

