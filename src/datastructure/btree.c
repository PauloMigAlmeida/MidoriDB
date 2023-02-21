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

int btree_insert(struct btree_head *head, void *key, void *val)
{

}

void* btree_remove(struct btree_head *head, unsigned long *key)
{

	return head;
}

