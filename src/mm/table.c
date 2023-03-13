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
	ret->free_dtbkl_offset = 0;

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


bool table_insert_row(struct table *table, void *data, size_t len)
{
	/* sanity checks */
	if (!table || !data || len == 0)
		return false;

	if (pthread_mutex_lock(&table->mutex))
		return false;

	/*
	 * check whether the data we are trying to insert is beyond what this table is
	 * meant hold - most likely someone has made a mistake for that to be the case.
	 * Then again, this is C.. you can never be "too safe" under any circumstance
	 */
	size_t expected_row_size = 0;
	for (int i = 0; i < table->column_count; i++)
		expected_row_size += table->columns[i].precision;

	if (len != expected_row_size)
		return false;

	/* is there enough space to insert that into an existing datablock, if not alloc a new one */
	struct datablock *block;
	if ((table->free_dtbkl_offset + len) >= DATABLOCK_PAGE_SIZE) {
		// Notes to myself, paulo, you should test the crap out of that..
		// TODO add some sort of POISON/EOF so when reading the datablock
		// we would know that it's time to  go to the next datablock
		//
		// We also have to differentiate between EOF and data deleted.. this will
		// happen once we implement delete operations
		if (!(block = datablock_alloc(table->datablock_head)))
			goto err_cleanup_mutex;
	} else {
		block = list_entry(table->datablock_head, typeof(*block), head);
	}

	memcpy(&block->data[table->free_dtbkl_offset], data, len);
	table->free_dtbkl_offset += len;

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
