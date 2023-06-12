/*
 * database.c
 *
 *  Created on: 8/06/2023
 *      Author: paulo
 */

#include <engine/database.h>

int database_open(struct database *db)
{
	/* sanity check */
	BUG_ON(!db);

	db->tables = zalloc(sizeof(*db->tables));
	if (!db->tables)
		goto err;

	if (!hashtable_init(db->tables, &hashtable_str_compare, &hashtable_str_hash))
		goto err_ht_init;

	return MIDORIDB_OK;

	err_ht_init:
	free(db->tables);
	err:
	return -MIDORIDB_ERROR;
}

void free_table(struct hashtable *hashtable, const void *key, size_t klen, const void *value, size_t vlen, void *arg)
{
	struct hashtable_entry *entry = NULL;

	UNUSED(vlen);
	UNUSED(arg);

	table_destroy((struct table**)value);

	entry = hashtable_remove(hashtable, key, klen);
	hashtable_free_entry(entry);
}

void database_close(struct database *db)
{

	/* sanity check */
	BUG_ON(!db);

	hashtable_foreach(db->tables, &free_table, NULL);
	hashtable_free(db->tables);
	free(db->tables);
	db->tables = NULL;
}

int database_table_add(struct database *db, struct table *table)
{
	int rc = MIDORIDB_OK;

	/* sanity check */
	BUG_ON(!db || !table);

	if (pthread_mutex_lock(&db->mutex)) {
		rc = -MIDORIDB_INTERNAL;
		goto err;
	}

	/* check if table exists */
	if (hashtable_get(db->tables, table->name, sizeof(table->name))) {
		rc = -MIDORIDB_ERROR;
		goto err_ht_dup;
	}

	if (!hashtable_put(db->tables, table->name, sizeof(table->name), &table, sizeof(uintptr_t))) {
		rc = -MIDORIDB_NOMEM;
		goto err_ht_put;
	}

	if (pthread_mutex_unlock(&db->mutex)) {
		rc = -MIDORIDB_INTERNAL;
		goto err_mt_unlock;
	}

	return rc;

	err_mt_unlock:
	err_ht_put:
	err_ht_dup:
	pthread_mutex_unlock(&db->mutex);
	err:
	return rc;

}

