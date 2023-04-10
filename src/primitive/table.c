/*
 * table.c
 *
 *  Created on: 4/03/2023
 *      Author: paulo
 */

#include <primitive/table.h>
#include <primitive/column.h>
#include <primitive/row.h>

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
		goto err_datablock;

	return ret;

	err_datablock:
	free(ret->datablock_head);
	err_free:
	free(ret);
	return NULL;
}

static void __free_datablock_content(struct table *table, struct datablock *block)
{
	size_t row_size = table_calc_row_size(table);
	for (size_t i = 0; i < (DATABLOCK_PAGE_SIZE / row_size); i++) {
		struct row *row = (struct row*)&block->data[row_size * i];
		table_free_row_content(table, row);
	}
}

bool table_destroy(struct table **table)
{
	struct datablock *entry = NULL;
	struct list_head *pos = NULL;
	struct list_head *tmp_pos = NULL;

	if (!(*table))
		return false;

	/* destroy table lock */
	if (pthread_mutex_destroy(&(*table)->mutex))
		return false;

	/* destroy all datablocks if any */
	list_for_each_safe(pos, tmp_pos, (*table)->datablock_head)
	{
		entry = list_entry(pos, typeof(*entry), head);

		/* free variable precision value as they are alloc'ed separately */
		__free_datablock_content(*table, entry);
		datablock_free(entry);
	}
	free((*table)->datablock_head);

	/* destroy table */
	free(*table);
	*table = NULL;

	return true;
}

void table_datablock_init(struct datablock *block, size_t offset, size_t row_size)
{
	for (size_t i = offset / row_size; i < DATABLOCK_PAGE_SIZE / row_size; i++) {
		struct row *row = (struct row*)&block->data[i * row_size];
		row->flags.empty = true;
		row->flags.deleted = false;
		memzero((char* )row + sizeof(row->flags), row_size - offsetof(typeof(*row), null_bitmap));
	}
}
