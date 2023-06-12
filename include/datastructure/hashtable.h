/*
 * hashtable.h
 *
 *  Created on: 2/06/2023
 *      Author: paulo
 */

#ifndef INCLUDE_DATASTRUCTURE_HASHTABLE_H_
#define INCLUDE_DATASTRUCTURE_HASHTABLE_H_

#include <compiler/common.h>
#include <datastructure/linkedlist.h>

struct hashtable;

typedef bool (*hashtable_compare_fn)(const void *key1, size_t key1_len, const void *key2, size_t key2_len);
typedef size_t (*hashtable_hash_fn)(const void *key, size_t key_len);
typedef void (hashtable_callback_fn)(struct hashtable* hashtable, const void *key, size_t klen, const void *value, size_t vlen, void *arg);

/* dynamic-sized hash table */
struct hashtable {
	size_t count;
	size_t capacity;
	char* array;
	hashtable_compare_fn compare_fn;
	hashtable_hash_fn hash_fn;
};

struct hashtable_key {
	void *content;
	size_t len;
};

struct hashtable_value {
	void *content;
	size_t len;
};

struct hashtable_entry {
	struct hashtable_key key;
	struct hashtable_value value;
	struct list_head head;
};

/**
 * hashtable_init - initialise hash table
 * @hashtable: hash table to be initialised
 *
 * this function returns true if hash table could be initialised, false otherwise
 */
bool __must_check hashtable_init(struct hashtable *hashtable,
		hashtable_compare_fn compare_fn,
		hashtable_hash_fn hash_fn);

/**
 * hashtable_free - free hash table
 * @hashtable: hash table to be freed
 */
void hashtable_free(struct hashtable *hashtable);

/**
 * hashtable_put - put content into hash table
 * @hashtable: hash table reference
 * @key: key to be inserted
 * @key_len: size of key in bytes
 * @value: value to be inserted
 * @value_len: size of value in bytes
 * 
 * this function returns true if content could be inserted, false otherwise
 */
bool __must_check hashtable_put(struct hashtable *hashtable,
		const void *key, size_t key_len,
		const void *value, size_t value_len);

/**
 * hashtable_get - get content from hash table
 * @hashtable: hash table reference
 * @key: key to be searched
 * @key_len: size of key in bytes
 * 
 * this function returns a pointer to the value if key is found, NULL otherwise
 */
struct hashtable_value* __must_check hashtable_get(struct hashtable *hashtable, const void *key, size_t key_len);

/**
 * hashtable_remove - remove content from hash table
 * @hashtable: hash table reference
 * @key: key to be removed
 * @key_len: size of key in bytes
 * 
 * this function returns entry pointer  if content could be removed, NULL otherwise.
 * NOTE.: It's callers responsibility to free the pointer returned.
 */
struct hashtable_entry* __must_check hashtable_remove(struct hashtable *hashtable, const void *key, size_t key_len);

/**
 * hashtable_foreach - iterate over hash table
 * @hashtable: hash table reference
 * @callback: callback function to be called for each element 
 * @arg: argument to be passed over to each invocation of the callback function
 */
void hashtable_foreach(struct hashtable *hashtable, hashtable_callback_fn *callback, void *arg);


/**
 * hashtable_str_compare - compare string keys
 * @key1: first key
 * @key1_len: size of first key in bytes
 * @key2: second key
 * @key2_len: size of second key in bytes
 * 
 * this function returns true if keys are equal, false otherwise
 */
bool hashtable_str_compare(const void *key1, size_t key1_len, const void *key2, size_t key2_len);

/**
 * hashtable_str_hash - hash string key
 * @key: key to be hashed
 * @key_len: size of key in bytes
 * 
 * this function returns the hash value of the key
 */
size_t hashtable_str_hash(const void *key, size_t key_len);

/**
 * hashtable_free_entry - free hashtable entry
 * @entry: entry to be freed
 *
 * Use this function with caution. You still responsible for freeing the content that either
 * key or value pointed two if that's a complex type.
 */
void hashtable_free_entry(struct hashtable_entry* entry);

#endif /* INCLUDE_DATASTRUCTURE_HASHTABLE_H_ */
