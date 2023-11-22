/*
 * hashtable.c
 *
 *  Created on: 2/06/2023
 *      Author: paulo
 */

#include <datastructure/hashtable.h>

#define HASHTABLE_DEFAULT_CAPACITY 	16
#define HASHTABLE_LOAD_FACTOR		0.5

static bool array_init(char *array, size_t capacity)
{
	struct list_head **ptr;

	for (size_t i = 0; i < capacity; i++) {
		ptr = (struct list_head**)(array + i * sizeof(uintptr_t));
		*ptr = zalloc(sizeof(struct list_head));
		if (!*ptr)
			goto err;

		list_head_init(*ptr);
	}

	return true;

err:
	for (size_t i = 0; i < capacity; i++) {
		ptr = (struct list_head**)(array + i * sizeof(uintptr_t));
		if (*ptr) {
			free(*ptr);
			*ptr = NULL;
		}
	}
	return false;
}

bool hashtable_init(struct hashtable *hashtable, hashtable_compare_fn compare_fn, hashtable_hash_fn hash_fn)
{
	/* sanity checks */
	BUG_ON(!hashtable || !compare_fn || !hash_fn);

	hashtable->capacity = HASHTABLE_DEFAULT_CAPACITY;
	hashtable->compare_fn = compare_fn;
	hashtable->hash_fn = hash_fn;
	hashtable->count = 0;

	hashtable->array = zalloc(sizeof(uintptr_t) * hashtable->capacity);
	if (!hashtable->array)
		goto err;

	if (!array_init(hashtable->array, hashtable->capacity))
		goto err_arr_init;

	return true;

err_arr_init:
	free(hashtable->array);
err:
	return false;

}

void hashtable_free(struct hashtable *hashtable)
{
	struct list_head *ptr;

	/* sanity checks */
	BUG_ON(!hashtable || hashtable->count != 0);

	for (size_t i = 0; i < hashtable->capacity; i++) {
		ptr = *(struct list_head**)(hashtable->array + i * sizeof(struct hashtable_entry*));
		BUG_ON(!list_is_empty(ptr));

		free(ptr);
		ptr = NULL;
	}

	free(hashtable->array);
	hashtable->array = NULL;
}

static void hashtable_resize(struct hashtable *hashtable)
{
	char *new_array;
	double load_factor;
	size_t new_capacity;
	size_t hash_idx;
	struct hashtable_entry *entry = NULL;
	struct list_head **ptr_old;
	struct list_head **ptr_new;
	struct list_head *pos = NULL;
	struct list_head *tmp_pos = NULL;

	load_factor = (double)hashtable->count / (double)hashtable->capacity;

	if (load_factor < HASHTABLE_LOAD_FACTOR)
		return;

	new_capacity = hashtable->capacity * 2;
	new_array = zalloc(sizeof(struct hashtable_entry*) * new_capacity);
	if (!new_array)
		return;

	if (!array_init(new_array, new_capacity)) {
		free(new_array);
		return;
	}

	for (size_t i = 0; i < hashtable->capacity; i++) {
		ptr_old = (struct list_head**)(hashtable->array + i * sizeof(uintptr_t));
		list_for_each_safe(pos, tmp_pos, *ptr_old)
		{
			entry = list_entry(pos, typeof(*entry), head);
			hash_idx = hashtable->hash_fn(entry->key.content, entry->key.len) % new_capacity;
			ptr_new = (struct list_head**)(new_array + hash_idx * sizeof(uintptr_t));

			list_del(&entry->head);
			list_add(&entry->head, *ptr_new);
		}
		free(*ptr_old);
	}

	free(hashtable->array);
	hashtable->array = new_array;
	hashtable->capacity = new_capacity;

}

bool hashtable_put(struct hashtable *hashtable, const void *key, size_t key_len, const void *value, size_t value_len)
{
	size_t hash_idx;
	struct list_head **ptr;
	struct list_head *pos = NULL;
	struct hashtable_entry *new;
	struct hashtable_entry *entry = NULL;

	/* sanity checks */
	BUG_ON(!hashtable || !key || !value || key_len == 0 || value_len == 0);

	new = zalloc(sizeof(struct hashtable_entry));
	if (!new)
		goto err;

	new->key.content = zalloc(key_len);
	if (!new->key.content)
		goto err_key;

	new->value.content = zalloc(value_len);
	if (!new->value.content)
		goto err_value;

	memcpy(new->key.content, key, key_len);
	memcpy(new->value.content, value, value_len);
	new->key.len = key_len;
	new->value.len = value_len;
	list_head_init(&new->head);

	hash_idx = hashtable->hash_fn(key, key_len) % hashtable->capacity;

	ptr = (struct list_head**)(hashtable->array + hash_idx * sizeof(struct hashtable_entry*));

	/* check if key already exists */
	list_for_each(pos, *ptr)
	{
		entry = list_entry(pos, typeof(*entry), head);
		if (hashtable->compare_fn(entry->key.content, entry->key.len, new->key.content, new->key.len))
			goto err_duplicate;
	}

	list_add(&new->head, *ptr);
	hashtable->count++;

	/* resize hashtable if needed */
	hashtable_resize(hashtable);

	return true;

err_duplicate:
	free(new->value.content);
err_value:
	free(new->key.content);
err_key:
	free(new);
err:
	return false;
}

struct hashtable_value* hashtable_get(struct hashtable *hashtable, const void *key, size_t key_len)
{
	struct list_head *ptr;
	struct list_head *pos = NULL;
	struct hashtable_entry *entry = NULL;
	size_t hash_idx;

	/* sanity checks */
	BUG_ON(!hashtable || !key || key_len == 0);

	hash_idx = hashtable->hash_fn(key, key_len) % hashtable->capacity;

	ptr = *(struct list_head**)(hashtable->array + hash_idx * sizeof(struct hashtable_entry*));

	list_for_each(pos, ptr)
	{
		entry = list_entry(pos, typeof(*entry), head);
		if (hashtable->compare_fn(entry->key.content, entry->key.len, key, key_len))
			return &entry->value;
	}

	return NULL;
}

struct hashtable_entry* hashtable_remove(struct hashtable *hashtable, const void *key, size_t key_len)
{
	struct list_head **ptr;
	struct list_head *pos = NULL;
	struct list_head *tmp_pos = NULL;
	struct hashtable_entry *entry = NULL;
	size_t hash_idx;

	/* sanity checks */
	BUG_ON(!hashtable || !key || key_len == 0);

	hash_idx = hashtable->hash_fn(key, key_len) % hashtable->capacity;

	ptr = (struct list_head**)(hashtable->array + hash_idx * sizeof(struct hashtable_entry*));

	list_for_each_safe(pos, tmp_pos, *ptr)
	{
		entry = list_entry(pos, typeof(*entry), head);
		if (hashtable->compare_fn(entry->key.content, entry->key.len, key, key_len)) {
			list_del(pos);
			hashtable->count--;
			return entry;
		}
	}

	return NULL;
}

void hashtable_foreach(struct hashtable *hashtable, hashtable_callback_fn *callback, void *arg)
{
	struct list_head **ptr;
	struct list_head *pos = NULL;
	struct list_head *tmp_pos = NULL;
	struct hashtable_entry *entry = NULL;

	for (size_t i = 0; i < hashtable->capacity; i++) {
		ptr = (struct list_head**)(hashtable->array + i * sizeof(uintptr_t));
		list_for_each_safe(pos, tmp_pos, *ptr)
		{
			entry = list_entry(pos, typeof(*entry), head);
			callback(hashtable, entry->key.content, entry->key.len, entry->value.content,
					entry->value.len,
					arg);
		}
	}
}

bool hashtable_str_compare(const void *key1, size_t key1_len, const void *key2, size_t key2_len)
{
	/* sanity checks */
	BUG_ON(!key1 || !key2 || key1_len == 0 || key2_len == 0);

	return strcmp(key1, key2) == 0;
}

size_t hashtable_str_hash(const void *key, size_t key_len)
{
	/* sanity checks */
	BUG_ON(!key || key_len == 0);

	size_t hash = 5381;
	const char *str = key;

	for (size_t i = 0; i < key_len; i++)
		hash = ((hash << 5) + hash) + str[i];

	return hash;
}

void hashtable_free_entry(struct hashtable_entry *entry)
{
	free(entry->key.content);
	free(entry->value.content);
	free(entry);
}
