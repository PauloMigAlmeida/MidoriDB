/*
 * database.c
 *
 *  Created on: 8/06/2023
 *      Author: paulo
 */

#include <engine/database.h>

bool database_open(struct database *db)
{
	/* sanity check */
	BUG_ON(!db);

	db->tables = zalloc(sizeof(*db->tables));
	if (!db->tables)
		goto err;

	if (!hashtable_init(db->tables, &hashtable_str_compare, &hashtable_str_hash))
		goto err_ht_init;

	return true;

	err_ht_init:
	free(db->tables);
	err:
	return false;
}

int free_table(struct hashtable *hashtable, const void *key, size_t klen, const void *value, size_t vlen, void *arg)
{
	struct hashtable_entry *entry = NULL;

	UNUSED(vlen);
	UNUSED(arg);

	table_destroy((struct table**)value);

	entry = hashtable_remove(hashtable, key, klen);
	hashtable_free_entry(entry);

	return 0;
}

void database_close(struct database *db)
{

	/* sanity check */
	BUG_ON(!db);

	//TODO add return type to hashtable_foreach -> it's useless as it is.
	hashtable_foreach(db->tables, &free_table, NULL);
	hashtable_free(db->tables);
	free(db->tables);
	db->tables = NULL;
}

bool database_add_table(struct database *db, struct table *table)
{
	bool ret;
	/* sanity check */
	BUG_ON(!db || !table);

	if (pthread_mutex_lock(&db->mutex))
		return false;

	/* check if table exists */
	if (hashtable_get(db->tables, table->name, sizeof(table->name)))
		return false;

	ret = hashtable_put(db->tables, table->name, sizeof(table->name), &table, sizeof(uintptr_t));

	if (pthread_mutex_unlock(&db->mutex))
		return false;

	return ret;

}
