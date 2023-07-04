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

static void free_table(struct hashtable *hashtable, const void *key, size_t klen, const void *value, size_t vlen, void *arg)
{
	struct hashtable_entry *entry = NULL;

	UNUSED(vlen);
	UNUSED(arg);

	// if we can't destroy a table, then a memory leak is guaranteed.
	// usually this can happen when we make mistakes regarding table_lock/table_unlock calls.
	BUG_ON(!table_destroy((struct table**)value));

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

int database_lock(struct database *db)
{
	if (pthread_mutex_lock(&db->mutex))
		return -MIDORIDB_INTERNAL;

	return MIDORIDB_OK;
}

int database_unlock(struct database *db)
{
	if (pthread_mutex_unlock(&db->mutex))
		return -MIDORIDB_INTERNAL;

	return MIDORIDB_OK;
}

int database_table_add(struct database *db, struct table *table)
{
	int rc = MIDORIDB_OK;

	/* sanity check */
	BUG_ON(!db || !table);

	/* check if table exists */
	if (database_table_exists(db, table->name)) {
		rc = -MIDORIDB_ERROR;
		goto err;
	}

	if (!hashtable_put(db->tables, table->name, sizeof(table->name), &table, sizeof(uintptr_t))) {
		rc = -MIDORIDB_NOMEM;
		goto err_ht_put;
	}

	return rc;

err_ht_put:
err:
	return rc;
}

struct table* database_table_get(struct database *db, char *table_name)
{
	struct hashtable_value *entry;
	entry = hashtable_get(db->tables, table_name, MIN(strlen(table_name) + 1, sizeof(((struct table* )0)->name)));
	return entry ? *(struct table**)entry->content : NULL;
}

bool database_table_exists(struct database *db, char *table_name)
{
	return database_table_get(db, table_name) != NULL;
}

