#ifndef DATASTRUCTURE_BTREE_H
#define DATASTRUCTURE_BTREE_H

#include "compiler/common.h"

struct btree_node_tuple {
	void *key;
	void *value;
};

struct btree_node {
	struct btree_node_tuple *keys;
	int key_count;
	struct btree_node *children;
	/*TODO check if we need that... apparently other implementations use
	 * "n" to interchangeably denote number of keys and number of children...
	 * I'm not convinced that I need to keep track of both
	 */
	int children_count;
	bool is_leaf;
};

/**
 * struct btree_head - btree head
 *
 * @node: the first node in the tree
 * @keylen: number of keys each node can hold
 * @cmp_fn: compare function
 */
struct btree_head {
	struct btree_node *root;
	int min_degree;
	size_t key_size;
	size_t val_size;
	int (*cmp_fn)(void *, void *);
};

/**
 * btree_init - initialise a btree
 *
 * @min_degree: number of keys each node can hold
 */
struct btree_head *btree_init(int min_degree,
			      size_t key_size,
			      size_t val_size,
			      int (*cmp_fn)(void *, void *));

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
void *btree_lookup(struct btree_head *head, void *key);


/**
 * btree_update - update an entry in the btree
 *
 * @head: the btree to update
 * @key: the key to update
 * @val: the value to change it to (must not be %NULL)
 */
bool btree_update(struct btree_head *head, unsigned long *key, void *val);

/**
 * btree_insert- insert an entry in the btree
 *
 * @head: the btree to update
 * @key: the key to insert
 * @val: the value to be inserted (must not be %NULL)
 */
int btree_insert(struct btree_head *head, void *key, void *val);

/**
 * btree_remove - remove an entry from the btree
 *
 * @head: the btree to update
 * @key: the key to remove
 *
 * This function returns the removed entry, or %NULL if the key
 * could not be found.
 */
void *btree_remove(struct btree_head *head, unsigned long *key);


int btree_cmp_str(void *key1, void *key2);
int btree_cmp_ul(void *key1, void *key2);

#endif /* DATASTRUCTURE_BTREE_H */
