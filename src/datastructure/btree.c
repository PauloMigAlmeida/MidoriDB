#include "datastructure/btree.h"

int btree_cmp_str(void *key1, void *key2)
{
	return strcmp(key1, key2);
}

int btree_cmp_ul(void *key1, void *key2)
{
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

struct btree_head* btree_init(int min_degree,
		size_t key_size,
		size_t val_size,
		int (*cmp_fn)(void*, void*))
{
	struct btree_head *head = NULL;

	BUG_ON(min_degree < 2);
	BUG_ON(!cmp_fn);

	if ((head = malloc(sizeof(*head)))) {
		memset(head, 0, sizeof(*head));
		head->min_degree = min_degree;
		head->key_size = key_size;
		head->val_size = val_size;
		head->cmp_fn = cmp_fn;
	} else {
		die("couldn't alloc mem for btree");
	}

	return head;
}

static void __btree_destroy(struct btree_head *head, struct btree_node *node)
{
	for (int i = 0; i < node->key_count; i++) {
		free(node->keys[i].key);
		free(node->keys[i].value);
	}
	free(node->keys);

	for (int i = 0; i < node->children_count; i++) {
		__btree_destroy(head, node->children + i);
	}
	free(node->children);

	free(node);
}

void btree_destroy(struct btree_head **head)
{
	if ((*head)->root)
		__btree_destroy(*head, (*head)->root);

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
	if (head->root)
		return btree_search(head, head->root, key);
	else
		return NULL; /* btree hasn't been initialised yet */
}

bool btree_update(struct btree_head *head, unsigned long *key, void *val)
{
	return true;
}

static struct btree_node* btree_node_alloc(struct btree_head *head)
{
	struct btree_node *node;
	size_t max_degree;

	max_degree = 2 * head->min_degree - 1;

	if (!(node = malloc(sizeof(*node))))
		goto err_node;

	if (!(node->keys = calloc(max_degree, head->key_size)))
		goto err_keys;

	if (!(node->children = calloc(max_degree, sizeof(*node))))
		goto err_children;

	node->key_count = 0;
	node->children_count = 0;
	node->is_leaf = false;
	return node;

	err_children: free(node->keys);
	err_keys: free(node);
	err_node: return NULL;
}

static int btree_split_child(struct btree_head *head, struct btree_node *x, int i)
{
//	// Create a new node which is going to store (t-1) keys
//	// of y
//	BTreeNode *z = new BTreeNode(y->t, y->leaf);
	struct btree_node *y;
	struct btree_node *z;

	y = &x->children[i];

	if (!(z = btree_node_alloc(head)))
		goto err;
	z->is_leaf = y->is_leaf;
	//	z->n = t - 1;
	z->key_count = head->min_degree - 1;

//	// Copy the last (t-1) keys of y to z
//	for (int j = 0; j < t-1; j++)
//	z->keys[j] = y->keys[j+t];

	for (int j = 0; j < head->min_degree - 1; j++)
		z->keys[j] = y->keys[j + head->min_degree];

//	// Copy the last t children of y to z
//	if (y->leaf == false)
//	{
//	for (int j = 0; j < t; j++)
//	    z->C[j] = y->C[j+t];
//	}
//

	if (!y->is_leaf) {
		for (int j = 0; j < head->min_degree; j++)
			z->children[j] = y->children[j + head->min_degree];
	}

//	// Reduce the number of keys in y
//	y->n = t - 1;
	y->key_count = head->min_degree - 1;
//
//	// Since this node is going to have a new child,
//	// create space of new child
//	for (int j = n; j >= i+1; j--)
//	C[j+1] = C[j];

	for (int j = x->key_count; j >= i + 1; j--)
		x->children[j + 1] = x->children[j];
//
//	// Link the new child to this node
//	C[i+1] = z;
	//TODO not convinved that memcpy is to be employed here
	memcpy(&x->children[i + 1], z, sizeof(struct btree_node));
//
//	// A key of y will move to this node. Find the location of
//	// new key and move all greater keys one space ahead
//	for (int j = n-1; j >= i; j--)
//	keys[j+1] = keys[j];

	for (int j = x->key_count - 1; j >= i; j--)
		x->keys[j + 1] = x->keys[j];
//
//	// Copy the middle key of y to this node
//	keys[i] = y->keys[t-1];

	x->keys[i] = y->keys[head->min_degree - 1];
//
//	// Increment count of keys in this node
//	n = n + 1;

	x->key_count++;
	return 0;

	err:
	return -EMDNOMEM;
}

static struct btree_node* btree_split_root(struct btree_head *head)
{
	struct btree_node *s;
	if (!(s = btree_node_alloc(head)))
		die("couldn't allocate mem");

	s->is_leaf = false;
	s->key_count = 0;
	memcpy(&s->children[0], head->root, sizeof(struct btree_node));
	head->root = s;
	btree_split_child(head, s, 0);

	return s;
}

static void btree_insert_nonfull(struct btree_head *head, struct btree_node *x, void *key, void *val)
{
	int i;

	i = x->key_count - 1;

	if (x->is_leaf) {
		while (i >= 0 && head->cmp_fn(x->keys[i].key, key) > 0) {
			x->keys[i + 1] = x->keys[i];
			i--;
		}

		// Insert the new key at found location
		memcpy(x->keys[i + 1].key, key, head->key_size);
		memcpy(x->keys[i + 1].value, val, head->val_size);
		x->key_count++;
	} else {
		// Find the child which is going to have the new key
		while (i >= 0 && head->cmp_fn(x->keys[i].key, key) > 0)
			i--;

		// See if the found child is full
		if (x->children[i + 1].key_count == 2 * head->min_degree - 1) {

			// If the child is full, then split it
			btree_split_child(head, x, i + 1);

			// After split, the middle key of C[i] goes up and
			// C[i] is splitted into two.  See which of the two
			// is going to have the new key
			if (head->cmp_fn(x->keys[i + 1].key, key) < 0)
				i++;
		}
		btree_insert_nonfull(head, &x->children[i + 1], key, val);
	}
}

int btree_insert(struct btree_head *head, void *key, void *val)
{
	struct btree_node *node;
	void *tmp_key, *tmp_val;

	if (!head->root) {

		if (!(node = btree_node_alloc(head)))
			goto err;
		if (!(tmp_key = malloc(head->key_size)))
			goto err_node;
		if (!(tmp_val = malloc(head->val_size)))
			goto err_key;

		memcpy(tmp_key, key, head->key_size);
		memcpy(tmp_val, val, head->val_size);

		node->keys[0].key = tmp_key;
		node->keys[0].value = tmp_val;
		node->is_leaf = true;
		node->key_count = 1;
		head->root = node;

	} else if (head->root->key_count == 2 * head->min_degree - 1) {

		node = btree_split_root(head);
		btree_split_child(head, node, 0);
		btree_insert_nonfull(head, node, key, val);

	} else {
		btree_insert_nonfull(head, head->root, key, val);
	}

	return 0;

	err_key:
	free(tmp_key);
	err_node:
	free(node);
	err:
	return -EMDNOMEM;
}

void* btree_remove(struct btree_head *head, unsigned long *key)
{
	return head;
}

