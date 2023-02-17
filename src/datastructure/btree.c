#include "datastructure/btree.h"

struct btree_head *btree_init(unsigned int keylen) {
	struct btree_head *head = NULL;
	if ((head = malloc(sizeof(*head)))) {
		memset(head, 0, sizeof(*head));
		head->keylen = keylen;
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

void *btree_remove(struct btree_head *head, unsigned long *key) {
	return head;
}

