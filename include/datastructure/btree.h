#ifndef DATASTRUCTURE_BTREE_H
#define DATASTRUCTURE_BTREE_H

#include "compiler/common.h"

/**
 * struct btree_head - btree head
 *
 * @node: the first node in the tree
 * @keylen: number of keys each node can hold
 */
struct btree_head {
	unsigned long *node;
	unsigned int keylen;
};

/**
 * btree_init - initialise a btree
 *
 * @keylen: number of keys each node can hold
 */
struct btree_head *btree_init(unsigned int keylen);

/**
 * btree_destroy - destroy btree recursively
 *
 * @head: the btree head to destroy
 */
void btree_destroy(struct btree_head *head);

/**
 * btree_lookup - look up a key in the btree
 *
 * @head: the btree to look in
 * @key: the key to look up
 *
 * This function returns the value for the given key, or NULL.
 */
void *btree_lookup(struct btree_head *head, unsigned long *key); 


/**
 * btree_update - update an entry in the btree
 *
 * @head: the btree to update
 * @key: the key to update
 * @val: the value to change it to (must not be %NULL)
 */
bool btree_update(struct btree_head *head, unsigned long *key, void *val);

/**
 * btree_remove - remove an entry from the btree
 *
 * @head: the btree to update
 * @geo: the btree geometry
 * @key: the key to remove
 *
 * This function returns the removed entry, or %NULL if the key
 * could not be found.
 */
void *btree_remove(struct btree_head *head, unsigned long *key);


#endif /* DATASTRUCTURE_BTREE_H */
