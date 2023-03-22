#ifndef DATASTRUCTURE_BTREE_H
#define DATASTRUCTURE_BTREE_H

#include "compiler/common.h"

struct btree_node_tuple {
	void *key;
	void *value;
};

struct btree_node {
	struct btree_node_tuple *keys;
	struct btree_node *children;
	int key_count;
	bool is_leaf;
};

/**
 * struct btree_head - btree head
 *
 * @min_degree: min number of keys each node can hold
 * @cmp_fn: pointer to a function that knows how to compare btree keys
 */
struct btree_head {
	struct btree_node *root;
	int min_degree;
	int (*cmp_fn)(void*, void*);
};

/**
 * btree_init - initialise a btree
 *
 * @min_degree: min number of keys each node can hold
 * @cmp_fn: pointer to a function that knows how to compare btree keys
 *
 * This function btree_head* or NULL if it fails to alloc memory
 */
struct btree_head* __must_check btree_init(int min_degree, int (*cmp_fn)(void*, void*));

/**
 * btree_destroy - destroy btree recursively
 *
 * @head: the btree head to destroy
 */
void btree_destroy(struct btree_head **head);

/**
 * btree_lookup - look up a key in the btree
 *
 * @head: the btree to look in
 * @key: the key to look up
 *
 * This function returns the value for the given key, or NULL.
 */
void* btree_lookup(struct btree_head *head, void *key);

/**
 * btree_update - update an entry in the btree
 *
 * @head: the btree to update
 * @key: the key to update
 * @val: the value to change it to (must not be %NULL)
 *
 * This function returns true if key is found and updated, false otherwise
 */
bool btree_update(struct btree_head *head, void *key, void *val);

/**
 * btree_insert- insert an entry in the btree
 *
 * @head: the btree to update
 * @key: the key to insert
 * @val: the value to be inserted (must not be %NULL)
 */
int __must_check btree_insert(struct btree_head *head, void *key, void *val);

/**
 * btree_remove - remove an entry from the btree
 *
 * @head: the btree to update
 * @key: the key to remove
 *
 * This function returns 0 if entry was removed (or not found) or a negative value if
 * an error happened
 */
int __must_check btree_remove(struct btree_head *head, void *key);

/**
 * btree_traverse - utility method for traversing the btree. Mostly used for debugging purposes
 *
 * @head: head of the btree
 */
void btree_traverse(struct btree_head *head);

int btree_cmp_str(void *key1, void *key2);
int btree_cmp_ul(void *key1, void *key2);

#endif /* DATASTRUCTURE_BTREE_H */
