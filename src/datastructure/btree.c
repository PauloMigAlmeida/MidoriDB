#include "datastructure/btree.h"

static void btree_node_remove(struct btree_head *head, struct btree_node *node, void *key);

int btree_cmp_str(void *key1, void *key2)
{
	return strcmp(key1, key2);
}

int btree_cmp_ul(void *key1, void *key2)
{
	uint64_t *k1 = key1;
	uint64_t *k2 = key2;
	if (*k1 < *k2)
		return -1;
	else if (*k1 > *k2)
		return 1;
	else
		return 0;
}

static inline int btree_max_degree(struct btree_head *head)
{
	return 2 * head->min_degree - 1;
}

struct btree_head* btree_init(int min_degree,
		size_t key_size,
		size_t val_size,
		int (*cmp_fn)(void*, void*))
{
	struct btree_head *head = NULL;

	BUG_ON(min_degree < 2);
	BUG_ON(!cmp_fn);

	if (!(head = zalloc(sizeof(*head))))
		die("couldn't alloc mem for btree");

	head->min_degree = min_degree;
	head->key_size = key_size;
	head->val_size = val_size;
	head->cmp_fn = cmp_fn;

	return head;
}

#define verbose_free(ptr) do { \
				/*printf("%p\n", (void*)ptr);*/\
				free(ptr);	\
			}while(0);

static void __btree_destroy(struct btree_head *head, struct btree_node *node)
{

	if (!node->is_leaf) {
		// A non-leaf node with k children contains k − 1 keys
		for (int i = 0; i < node->key_count + 1; i++) {
			__btree_destroy(head, &node->children[i]);
		}

	}

	verbose_free(node->children);
	verbose_free(node->keys);
}

void btree_destroy(struct btree_head **head)
{
	if ((*head)->root) {
		__btree_destroy(*head, (*head)->root);
		verbose_free((*head)->root);
	}

	free(*head);
	*head = NULL;
}

static void* btree_search(struct btree_head *head, struct btree_node *node, void *key)
{
	int i = 0;
	while (i < node->key_count && head->cmp_fn(key, node->keys[i].key) > 0)
		i++;

	if (head->cmp_fn(node->keys[i].key, key) == 0)
		return node->keys[i].value;

	if (node->is_leaf)
		return NULL;

	return btree_search(head, node->children + i, key);
}

void* btree_lookup(struct btree_head *head, void *key)
{
	if (!head->root)
		return NULL; /* btree hasn't been initialised yet */

	return btree_search(head, head->root, key);
}

bool btree_update(struct btree_head *head, unsigned long *key, void *val)
{
	(void)head;
	(void)key;
	(void)val;
	return true;
}

static struct btree_node* btree_node_alloc(struct btree_head *head)
{
	struct btree_node *node;
	int max_degree;

	max_degree = btree_max_degree(head) + 1 /* sentinel node */;

	if (!(node = zalloc(sizeof(*node))))
		goto err_node;

	if (!(node->keys = calloc(max_degree, sizeof(struct btree_node_tuple))))
		goto err_keys;

	if (!(node->children = calloc(max_degree, sizeof(struct btree_node))))
		goto err_children;

	node->is_leaf = true;
	return node;

	err_children: free(node->keys);
	err_keys: free(node);
	err_node: return NULL;
}

static int btree_node_copy(struct btree_head *head, struct btree_node *dst, struct btree_node *src)
{

	int max_degree = btree_max_degree(head) + 1 /* sentinel node */;

	dst->is_leaf = src->is_leaf;
	dst->key_count = src->key_count;

	if (!(dst->keys = calloc(max_degree, sizeof(struct btree_node_tuple))))
		goto err;

	memcpy(dst->keys, src->keys, max_degree * sizeof(struct btree_node_tuple));

	if (!(dst->children = calloc(max_degree, sizeof(struct btree_node))))
		goto err_children;

	if (!src->is_leaf) {
		for (int i = 0; i <= src->key_count; i++) {
			if (btree_node_copy(head, &dst->children[i], &src->children[i]) != 0)
				goto err_recur;
		}
	}

	return 0;

	err_recur: free(dst->children);
	err_children: free(dst->keys);
	err:
	printf("error while allocating mem");
	return -EMDNOMEM;
}

static int btree_split_child(struct btree_head *head, struct btree_node *x, int i)
{
	struct btree_node *y;
	struct btree_node *z;

	y = &x->children[i];

	if (!(z = btree_node_alloc(head)))
		goto err;

	z->is_leaf = y->is_leaf;
	z->key_count = head->min_degree - 1;

	for (int j = 0; j < head->min_degree - 1; j++) {
		z->keys[j] = y->keys[j + head->min_degree];
		memzero(&y->keys[j + head->min_degree], sizeof(struct btree_node_tuple));
	}

	if (!y->is_leaf) {
		for (int j = 0; j < head->min_degree; j++) {
			z->children[j] = y->children[j + head->min_degree];
			memzero(&y->children[j + head->min_degree], sizeof(struct btree_node));
		}

	}

	y->key_count = head->min_degree - 1;

	for (int j = x->key_count; j >= i + 1; j--)
		x->children[j + 1] = x->children[j];

	x->children[i + 1] = *z;

	for (int j = x->key_count - 1; j >= i; j--)
		x->keys[j + 1] = x->keys[j];

	x->keys[i] = y->keys[head->min_degree - 1];
	memzero(&y->keys[head->min_degree - 1], sizeof(struct btree_node_tuple));
	x->key_count++;

	free(z);
	return 0;

	err:
	return -EMDNOMEM;
}

static struct btree_node* btree_split_root(struct btree_head *head)
{
	struct btree_node *s;
	if (!(s = btree_node_alloc(head)))
		//TODO properly hand this
		die("couldn't allocate mem");

	s->is_leaf = false;
	s->key_count = 0;
	s->children[0] = *head->root;
	free(head->root);
	head->root = s;
	btree_split_child(head, s, 0);

	return s;
}

static int btree_insert_nonfull(struct btree_head *head, struct btree_node *x, void *key, void *val)
{
	int i;

	i = x->key_count - 1;

	if (x->is_leaf) {
		while (i >= 0 && head->cmp_fn(x->keys[i].key, key) > 0) {
			x->keys[i + 1] = x->keys[i];
			i--;
		}

		x->keys[i + 1].key = key;
		x->keys[i + 1].value = val;
		x->key_count++;
	} else {
		// Find the child which is going to have the new key
		while (i >= 0 && head->cmp_fn(x->keys[i].key, key) > 0)
			i--;

		// See if the found child is full
		if (x->children[i + 1].key_count == btree_max_degree(head)) {

			// If the child is full, then split it
			btree_split_child(head, x, i + 1);

			// After split, the middle key of C[i] goes up and
			// C[i] is split into two.  See which of the two
			// is going to have the new key
			if (head->cmp_fn(x->keys[i + 1].key, key) < 0)
				i++;
		}
		btree_insert_nonfull(head, &x->children[i + 1], key, val);
	}

	return 0;

}

int btree_insert(struct btree_head *head, void *key, void *val)
{
	struct btree_node *node;

	if (!head->root) {

		if (!(node = btree_node_alloc(head)))
			goto err;

		node->keys[0].key = key;
		node->keys[0].value = val;
		node->is_leaf = true;
		node->key_count = 1;
		head->root = node;

	} else if (head->root->key_count == btree_max_degree(head)) {

		node = btree_split_root(head);
		btree_insert_nonfull(head, node, key, val);

	} else {
		btree_insert_nonfull(head, head->root, key, val);
	}

	return 0;

	err:
	return -EMDNOMEM;
}

// A function to remove the idx-th key from this node - which is a leaf node
static void btree_node_removeFromLeaf(struct btree_head *head, struct btree_node *node, int idx)
{
	(void)(head);
// Move all the keys after the idx-th pos one place backward
	for (int i = idx + 1; i < node->key_count; ++i) {
		node->keys[i - 1] = node->keys[i];
		memzero(&node->keys[i], sizeof(struct btree_node_tuple));
	}

	node->key_count--;

}

// A function to get predecessor of keys[idx] TODO watch out
static struct btree_node_tuple btree_node_getPred(struct btree_node *node, int idx)
{
// Keep moving to the right most node until we reach a leaf
	struct btree_node *cur = &node->children[idx];
	while (!cur->is_leaf)
		cur = &cur->children[cur->key_count];

// Return the last key of the leaf
	return cur->keys[cur->key_count - 1];
}

static struct btree_node_tuple btree_node_getSucc(struct btree_node *node, int idx)
{

	// Keep moving the left most node starting from C[idx+1] until we reach a leaf
	struct btree_node *cur = &node->children[idx + 1];
	while (!cur->is_leaf)
		cur = &cur->children[0];

	// Return the first key of the leaf
	return cur->keys[0];
}

// A function to merge C[idx] with C[idx+1]
// C[idx+1] is freed after merging
static void btree_node_merge(struct btree_head *head, struct btree_node *node, int idx)
{
	struct btree_node *child, *sibling;

	child = &node->children[idx];
	sibling = &node->children[idx + 1];

	// Pulling a key from the current node and inserting it into (t-1)th
	// position of C[idx]
	child->keys[head->min_degree - 1] = node->keys[idx];

	// Copying the keys from C[idx+1] to C[idx] at the end
	for (int i = 0; i < sibling->key_count; ++i) {
		child->keys[i + head->min_degree] = sibling->keys[i];
		memzero(&sibling->keys[i], sizeof(struct btree_node_tuple));
	}

	// Copying the child pointers from C[idx+1] to C[idx]
	if (!child->is_leaf) {
		for (int i = 0; i <= sibling->key_count; ++i) {
			btree_node_copy(head, &child->children[i + head->min_degree], &sibling->children[i]);
		}
	}

	// Updating the key count of child and the current node
	child->key_count += sibling->key_count + 1;

	// Freeing the memory occupied by sibling
	__btree_destroy(head, sibling);

	// Moving all keys after idx in the current node one step before -
	// to fill the gap created by moving keys[idx] to C[idx]
	for (int i = idx + 1; i < node->key_count; ++i) {
		node->keys[i - 1] = node->keys[i];
		memzero(&node->keys[i], sizeof(struct btree_node_tuple));
	}

	// Moving the child pointers after (idx+1) in the current node one
	// step before
	for (int i = idx + 2; i <= node->key_count; ++i) {
		node->children[i - 1] = node->children[i];
	}

	node->key_count--;
}

// A function to remove the idx-th key from this node - which is a non-leaf node
static void btree_node_removeFromNonLeaf(struct btree_head *head, struct btree_node *node, int idx)
{

	struct btree_node_tuple k = node->keys[idx];

	// If the child that precedes k (C[idx]) has atleast t keys,
	// find the predecessor 'pred' of k in the subtree rooted at
	// C[idx]. Replace k by pred. Recursively delete pred
	// in C[idx]
	if (node->children[idx].key_count >= head->min_degree) {
		struct btree_node_tuple pred = btree_node_getPred(node, idx);
		node->keys[idx] = pred;
		btree_node_remove(head, &node->children[idx], pred.key);
	}

	// If the child C[idx] has less that t keys, examine C[idx+1].
	// If C[idx+1] has atleast t keys, find the successor 'succ' of k in
	// the subtree rooted at C[idx+1]
	// Replace k by succ
	// Recursively delete succ in C[idx+1]
	else if (node->children[idx + 1].key_count >= head->min_degree) {
		struct btree_node_tuple succ = btree_node_getSucc(node, idx);
		node->keys[idx] = succ;
		btree_node_remove(head, &node->children[idx + 1], succ.key);
	}

	// If both C[idx] and C[idx+1] has less that t keys,merge k and all of C[idx+1]
	// into C[idx]
	// Now C[idx] contains 2t-1 keys
	// Free C[idx+1] and recursively delete k from C[idx]
	else {
		btree_node_merge(head, node, idx);
		btree_node_remove(head, &node->children[idx], k.key);
	}
}

// A function to borrow a key from C[idx-1] and insert it
// into C[idx]
static void btree_node_borrowFromPrev(struct btree_node *node, int idx)
{

	struct btree_node *child, *sibling;

	child = node->children + idx;
	sibling = node->children + idx - 1;

	// The last key from C[idx-1] goes up to the parent and key[idx-1]
	// from parent is inserted as the first key in C[idx]. Thus, the  loses
	// sibling one key and child gains one key

	// Moving all key in C[idx] one step ahead
	for (int i = child->key_count - 1; i >= 0; --i)
		child->keys[i + 1] = child->keys[i];

	// If C[idx] is not a leaf, move all its child pointers one step ahead
	if (!child->is_leaf)
	{
		for (int i = child->key_count; i >= 0; --i)
			child->children[i + 1] = child->children[i];
	}

	// Setting child's first key equal to keys[idx-1] from the current node
	child->keys[0] = node->keys[idx - 1];

	// Moving sibling's last child as C[idx]'s first child
	if (!child->is_leaf)
		child->children[0] = sibling->children[sibling->key_count];

	// Moving the key from the sibling to the parent
	// This reduces the number of keys in the sibling
	node->keys[idx - 1] = sibling->keys[sibling->key_count - 1];

	child->key_count += 1;
	sibling->key_count -= 1;
}

// A function to borrow a key from the C[idx+1] and place
// it in C[idx]
static void btree_node_borrowFromNext(struct btree_node *node, int idx)
{

	struct btree_node *child, *sibling;

	child = &node->children[idx];
	sibling = &node->children[idx + 1];

	// keys[idx] is inserted as the last key in C[idx]
	child->keys[(child->key_count)] = node->keys[idx];

	// Sibling's first child is inserted as the last child
	// into C[idx]
	if (!(child->is_leaf))
		child->children[(child->key_count) + 1] = sibling->children[0];

	//The first key from sibling is inserted into keys[idx]
	node->keys[idx] = sibling->keys[0];

	// Moving all keys in sibling one step behind
	for (int i = 1; i < sibling->key_count; ++i)
		sibling->keys[i - 1] = sibling->keys[i];
	memzero(&sibling->keys[sibling->key_count - 1], sizeof(struct btree_node_tuple));

	// Moving the child pointers one step behind
	if (!sibling->is_leaf) {
		for (int i = 1; i <= sibling->key_count; ++i)
			sibling->children[i - 1] = sibling->children[i];
		memzero(&sibling->children[sibling->key_count], sizeof(struct btree_node));
	}

	// Increasing and decreasing the key count of C[idx] and C[idx+1]
	// respectively
	child->key_count += 1;
	sibling->key_count -= 1;
}

// A function to fill child C[idx] which has less than t-1 keys
static void btree_node_fill(struct btree_head *head, struct btree_node *node, int idx)
{

	// If the previous child(C[idx-1]) has more than t-1 keys, borrow a key
	// from that child
	if (idx != 0 && node->children[idx - 1].key_count >= head->min_degree) {
		btree_node_borrowFromPrev(node, idx);
	} else if (idx != node->key_count && node->children[idx + 1].key_count >= head->min_degree) {
		btree_node_borrowFromNext(node, idx);
	} else {
		if (idx != node->key_count)
			btree_node_merge(head, node, idx);
		else
			btree_node_merge(head, node, idx - 1);
	}
}

// A utility function that returns the index of the first key that is
// greater than or equal to k
static int btree_node_find_key(struct btree_head *head, struct btree_node *node, void *key)
{
	int i = 0;
	while (i < node->key_count && head->cmp_fn(node->keys[i].key, key) < 0)
		++i;
	return i;
}

// A function to remove the key k from the sub-tree rooted with this node
static void btree_node_remove(struct btree_head *head, struct btree_node *node, void *key)
{
	int idx = btree_node_find_key(head, node, key);

	// The key to be removed is present in this node
	if (idx < node->key_count && head->cmp_fn(node->keys[idx].key, key) == 0) {

		// If the node is a leaf node - removeFromLeaf is called
		// Otherwise, removeFromNonLeaf function is called
		if (node->is_leaf)
			btree_node_removeFromLeaf(head, node, idx);
		else
			btree_node_removeFromNonLeaf(head, node, idx);
	} else {

		// If this node is a leaf node, then the key is not present in tree
		if (node->is_leaf)
			return;

		// If the child where the key is supposed to exist has less that t keys,
		// we fill that child
		if (node->children[idx].key_count < head->min_degree)
			btree_node_fill(head, node, idx);

		// If the last child has been merged, it must have merged with the previous
		// child and so we recurse on the (idx-1)th child. Else, we recurse on the
		// (idx)th child which now has atleast t keys

//		if (idx == node->key_count && idx > node->key_count)
//			btree_node_remove(head, &node->children[idx - 1], key);
//		else
//			btree_node_remove(head, &node->children[idx], key);
		// experiment
		if (idx == node->key_count && idx > node->key_count)
			btree_node_remove(head, &node->children[idx - 1], key);
		else
			btree_node_remove(head, &node->children[idx], key);
	}
}

void btree_remove(struct btree_head *head, void *key)
{
	if (!head->root)
		return;

	// Call the remove function for root
	btree_node_remove(head, head->root, key);

	// If the root node has 0 keys, make its first child as the new root
	//  if it has a child, otherwise set root as NULL
	if (head->root->key_count == 0) {
		struct btree_node *tmp = head->root;
		if (head->root->is_leaf)
			head->root = NULL;
		else
			head->root = &head->root->children[0];

		// Free the old root TODO: check that during test
		__btree_destroy(head, tmp);

	}
}

