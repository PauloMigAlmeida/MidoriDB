/*
 * table.c
 *
 *  Created on: 4/03/2023
 *      Author: paulo
 */

#include "mm/table.h"

static inline bool __valid_name(char *name, size_t max_size)
{
	size_t arg_len;

	arg_len = strlen(name);
	if (arg_len == 0 || arg_len > max_size)
		return false;

	for (size_t i = 0; i < arg_len; i++) {
		char c = name[i];
		if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
				(c >= '0' && c <= '9' && i != 0) ||
				(c == '_' && i != 0)))
			return false;
	}
	return true;
}

static bool table_validate_name(char *name)
{
	return __valid_name(name, TABLE_MAX_NAME);
}

struct table* __must_check table_init(char *name)
{
	struct table *ret;

	if (!name)
		return NULL;

	ret = zalloc(sizeof(*ret));
	if (!ret)
		return NULL;

	ret->column_count = 0;

	if (!table_validate_name(name))
		goto err_free;

	strncpy(ret->name, name, TABLE_MAX_NAME);

	ret->datablock_head = datablock_init();
	if (!ret->datablock_head)
		goto err_free;

	if (pthread_mutex_init(&ret->mutex, NULL))
		goto err_mutex;

	return ret;

	err_mutex:
	free(ret->datablock_head);
	err_free:
	free(ret);
	return NULL;
}

bool table_destroy(struct table **table)
{
	struct datablock *entry = NULL;
	struct list_head *pos = NULL;

	if (!(*table))
		return false;

	/* destroy table lock */
	if (pthread_mutex_destroy(&(*table)->mutex))
		return false;

	/* destroy all datablocks if any */
	list_for_each(pos, (*table)->datablock_head)
	{
		entry = list_entry(pos, typeof(*entry), head);
		datablock_free(entry);
	}
	free((*table)->datablock_head);

	/* destroy table */
	free(*table);
	*table = NULL;

	return true;
}

static bool table_validate_column_name(char *name)
{
	return __valid_name(name, TABLE_MAX_COLUMN_NAME);
}

bool table_add_column(struct table *table, struct column *column)
{
	/* sanity checks */
	if (!table || !column)
		return false;

	/* check column name rules */
	if (!table_validate_column_name(column->name) || column->length < 1)
		return false;

	if (pthread_mutex_lock(&table->mutex))
		return false;

	/* check whether it is within the max limit for columns */
	if (table->column_count + 1 > TABLE_MAX_COLUMNS) {
		goto err_cleanup_mutex;
	}

	/* check if this column exists already */
	for (int i = 0; i < table->column_count; i++) {
		if (strcmp(table->columns[i].name, column->name) == 0)
			goto err_cleanup_mutex;
	}

	/* adding column to the table */
	memcpy(&table->columns[table->column_count], column, sizeof(*column));
	table->column_count++;

	if (pthread_mutex_unlock(&table->mutex))
		return false;

	return true;

	err_cleanup_mutex:
	/*
	 * pthread_mutex_unlock should fail if
	 * 	 - [EINVAL] The value specified for the argument is not correct.
	 * 	 - [EPERM]  The mutex is not currently held by the caller.
	 *
	 * In both cases, this is the symptom for something much worse, so in this case
	 * the program should die... and the developer blamed :)
	 */
	BUG_ON(pthread_mutex_unlock(&table->mutex));
	return false;

}
