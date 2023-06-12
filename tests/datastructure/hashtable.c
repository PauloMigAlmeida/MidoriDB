#include <tests/datastructure.h>
#include <datastructure/hashtable.h>
#include <lib/string.h>

static void free_str_entries(struct hashtable *hashtable, const void *key, size_t klen, const void *value, size_t vlen, void *arg)
{
	struct hashtable_entry *entry = NULL;
	UNUSED(arg);

	BUG_ON(!key || klen == 0 || !value || vlen == 0);
	entry = hashtable_remove(hashtable, key, klen);
	CU_ASSERT_PTR_NOT_NULL(entry);
	hashtable_free_entry(entry);
}

void test_hashtable_init(void)
{
	struct hashtable ht = {0};
	CU_ASSERT(hashtable_init(&ht, &hashtable_str_compare, &hashtable_str_hash));
	CU_ASSERT_PTR_NOT_NULL(ht.array);
	CU_ASSERT_PTR_NOT_NULL(ht.compare_fn);
	CU_ASSERT_PTR_NOT_NULL(ht.hash_fn);
	CU_ASSERT_EQUAL(ht.capacity, 16);
	CU_ASSERT_EQUAL(ht.count, 0);
	CU_ASSERT_PTR_NOT_NULL(&ht);
	hashtable_free(&ht);
}

void test_hashtable_free(void)
{
	struct hashtable ht = {0};
	CU_ASSERT(hashtable_init(&ht, &hashtable_str_compare, &hashtable_str_hash));
	CU_ASSERT_PTR_NOT_NULL(&ht);

	hashtable_free(&ht);
	CU_ASSERT_PTR_NULL(ht.array);
}

void test_hashtable_put(void)
{
	struct hashtable ht = {0};
	struct hashtable_value *entry = NULL;
	char str_key[11];
	char str_value[11];

	CU_ASSERT(hashtable_init(&ht, &hashtable_str_compare, &hashtable_str_hash));
	CU_ASSERT_PTR_NOT_NULL(&ht);

	/* force capacity to increase so we can test trick bugs :-) */
	for (int i = 0; i < 2000; i++) {
		strrand(str_key, ARR_SIZE(str_key) - 1);
		strrand(str_value, ARR_SIZE(str_value) - 1);
		CU_ASSERT(hashtable_put(&ht, str_key, sizeof(str_key), str_value, sizeof(str_value)));
		CU_ASSERT_EQUAL(ht.count, i + 1);

		entry = hashtable_get(&ht, str_key, sizeof(str_key));
		CU_ASSERT_PTR_NOT_NULL(entry);
		CU_ASSERT_STRING_EQUAL(entry->content, str_value);
		CU_ASSERT_EQUAL(ht.count, i + 1);

	}

	hashtable_foreach(&ht, &free_str_entries, NULL);
	hashtable_free(&ht);
}

void test_hashtable_get(void)
{
	struct hashtable ht = {0};
	char *key1 = "hello";
	char *key2 = "paulo";
	char *value1 = "world";
	struct hashtable_value *entry = NULL;

	CU_ASSERT(hashtable_init(&ht, &hashtable_str_compare, &hashtable_str_hash));
	CU_ASSERT_PTR_NOT_NULL(&ht);
	CU_ASSERT_EQUAL(ht.count, 0);

	/* item that exists */
	CU_ASSERT(hashtable_put(&ht, key1, sizeof(key1), value1, sizeof(value1)));
	CU_ASSERT_EQUAL(ht.count, 1);
	entry = hashtable_get(&ht, key1, sizeof(key1));
	CU_ASSERT_PTR_NOT_NULL(entry);
	CU_ASSERT_STRING_EQUAL(entry->content, value1);
	CU_ASSERT_EQUAL(ht.count, 1);

	/* item that doesn't exists */
	entry = hashtable_get(&ht, key2, sizeof(key2));
	CU_ASSERT_PTR_NULL(entry);
	CU_ASSERT_EQUAL(ht.count, 1);

	hashtable_foreach(&ht, &free_str_entries, NULL);
	hashtable_free(&ht);

}

void test_hashtable_remove(void)
{
	struct hashtable ht = {0};
	char *key1 = "hello";
	char *value1 = "world";
	char *key2 = "ola";
	struct hashtable_entry *entry = NULL;

	CU_ASSERT(hashtable_init(&ht, &hashtable_str_compare, &hashtable_str_hash));
	CU_ASSERT_PTR_NOT_NULL(&ht);
	CU_ASSERT_EQUAL(ht.count, 0);

	/* item that exists */
	CU_ASSERT(hashtable_put(&ht, key1, sizeof(key1), value1, sizeof(value1)));
	CU_ASSERT_EQUAL(ht.count, 1);
	entry = hashtable_remove(&ht, key1, sizeof(key1));
	CU_ASSERT_PTR_NOT_NULL(entry);
	CU_ASSERT_STRING_EQUAL(entry->key.content, key1);
	CU_ASSERT_STRING_EQUAL(entry->value.content, value1);
	CU_ASSERT_EQUAL(ht.count, 0);
	hashtable_free_entry(entry);

	/* item that doesn't exists */
	CU_ASSERT(hashtable_put(&ht, key1, sizeof(key1), value1, sizeof(value1)));
	CU_ASSERT_EQUAL(ht.count, 1);
	entry = hashtable_remove(&ht, key2, sizeof(key2));
	CU_ASSERT_PTR_NULL(entry);
	CU_ASSERT_EQUAL(ht.count, 1);

	hashtable_foreach(&ht, &free_str_entries, NULL);
	hashtable_free(&ht);

}

static void test_hashtable_iterate_callback(struct hashtable *hashtable, const void *key, size_t klen,
		const void *value, size_t vlen, void *arg)
{
	UNUSED(hashtable);
	UNUSED(key);
	UNUSED(klen);
	UNUSED(value);
	UNUSED(vlen);
	*((int*)arg) += 1;
}

void test_hashtable_iterate(void)
{
	struct hashtable ht = {0};
	char str_key[11];
	char str_value[11];
	int iterate_count = 0;

	CU_ASSERT(hashtable_init(&ht, &hashtable_str_compare, &hashtable_str_hash));
	CU_ASSERT_PTR_NOT_NULL(&ht);
	CU_ASSERT_EQUAL(ht.count, 0);

	for (int i = 0; i < 10; i++) {
		strrand(str_key, ARR_SIZE(str_key) - 1);
		strrand(str_value, ARR_SIZE(str_value) - 1);
		CU_ASSERT(hashtable_put(&ht, str_key, sizeof(str_key), str_value, sizeof(str_value)));
	}
	CU_ASSERT_EQUAL(ht.count, 10);

	CU_ASSERT_EQUAL(iterate_count, 0);
	hashtable_foreach(&ht, &test_hashtable_iterate_callback, &iterate_count);
	CU_ASSERT_EQUAL(iterate_count, 10);
	CU_ASSERT_EQUAL(ht.count, 10);

	hashtable_foreach(&ht, &free_str_entries, NULL);
	hashtable_free(&ht);
}
