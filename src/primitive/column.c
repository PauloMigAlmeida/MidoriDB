/*
 * column.c
 *
 *  Created on: 13/03/2023
 *      Author: paulo
 */

#include <primitive/table.h>

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

static bool table_validate_column(struct column *column)
{
	if (!__valid_name(column->name, TABLE_MAX_COLUMN_NAME))
		return false;

	if (column->precision < 1)
		return false;

	return true;
}

static bool datablock_add_column(struct table *table, size_t row_cur_size, size_t row_nxt_size)
{
	struct list_head *old_head, *new_head;
	struct list_head *old_pos, *new_pos, *tmp_pos;
	struct datablock *old_entry, *new_entry;
	size_t blk_offset;

	/* sanity checks */
	if (!table || row_cur_size == row_nxt_size)
		goto err;

	old_head = table->datablock_head;
	blk_offset = 0;
	old_entry = NULL;
	new_entry = NULL;

	/* alloc new datablock head in which data will live */
	new_head = datablock_init();
	if (!new_head)
		goto err;

	list_for_each(old_pos, old_head)
	{
		old_entry = list_entry(old_pos, typeof(*old_entry), head);

		// TODO when moving rows to different blocks, we have to update indexes to point to those
		for (size_t i = 0; i < (DATABLOCK_PAGE_SIZE / row_cur_size); i++) {
			struct row *row = (struct row*)&old_entry->data[i * row_cur_size];

			/* are we done yet ? */
			if (row->flags.empty)
				break;

			/* is new datablock head empty? can it fit into current data block? */
			if (!new_entry || (blk_offset + row_nxt_size) >= DATABLOCK_PAGE_SIZE) {
				new_entry = datablock_alloc(new_head);

				if (!new_entry)
					goto err_free;

				table_datablock_init(new_entry, row_nxt_size);
				blk_offset = 0;
			}

			/* copy row into new datablock */
			memcpy(&new_entry->data[blk_offset], row, row_cur_size);
			blk_offset += row_nxt_size;
		}
	}

	/* change table's datablock head */
	table->datablock_head = new_head;
	table->free_dtbkl_offset = blk_offset;

	/* free old head / old data block */
	list_for_each_safe(old_pos, tmp_pos, old_head)
	{
		datablock_free(list_entry(old_pos, typeof(*old_entry), head));
	}
	free(old_head);

	return true;

	err_free:
	list_for_each_safe(new_pos, tmp_pos, new_head)
	{
		datablock_free(list_entry(new_pos, typeof(*new_entry), head));
	}
	free(new_head);

	err:
	return false;
}

bool table_add_column(struct table *table, struct column *column)
{
	size_t row_cur_size;

	/* sanity checks */
	if (!table || !column)
		return false;

	/* check column name rules */
	if (!table_validate_column(column))
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

	row_cur_size = table_calc_row_size(table);

	/* adding column to the table */
	memcpy(&table->columns[table->column_count], column, sizeof(*column));
	table->column_count++;

	/* if table isn't empty than we can to rearrange rows in datablocks */
	if (!list_is_empty(table->datablock_head)) {
		if (!datablock_add_column(table, row_cur_size, table_calc_row_size(table)))
			goto err_cleanup_mutex;
	}

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

static void datablock_rem_column(struct table *table, size_t col_idx)
{
	struct list_head *head;
	struct list_head *pos;
	struct datablock *entry;
	struct column *column;
	struct row *row;
	size_t row_cur_size, row_new_size, row_data_size;
	size_t data_offset, blk_offset;
	size_t col_prec;

	head = table->datablock_head;
	column = &table->columns[col_idx];
	col_prec = table_calc_column_space(column);
	row_cur_size = table_calc_row_size(table);
	row_new_size = row_cur_size - col_prec;
	row_data_size = table_calc_row_data_size(table);
	data_offset = 0;
	entry = NULL;

	for (size_t i = 0; i < col_idx; i++) {
		data_offset += table_calc_column_space(&table->columns[i]);
	}

	list_for_each(pos, head)
	{
		entry = list_entry(pos, typeof(*entry), head);
		blk_offset = 0;

		// TODO we _may_ have to update indexes to point when making these changes
		for (size_t i = 0; i < (DATABLOCK_PAGE_SIZE / row_cur_size); i++) {
			row = (struct row*)&entry->data[i * row_cur_size];

			/* are we done yet? */
			if (row->flags.empty)
				break;

			/* if that held a var precision column, we need to free it */
			if (table_check_var_column(column)) {
				void **var_ptr = (void**)&row->data[data_offset];
				free(*var_ptr);
				memzero(var_ptr, sizeof(*var_ptr));
			}

			/* remove column data */
			memmove(&row->data[data_offset],
				&row->data[data_offset + col_prec],
				row_data_size - (data_offset + col_prec));

			/* make rows sit right next to each other now that row has a smaller size */
			memmove(&entry->data[blk_offset], row, row_new_size);
			blk_offset += row_new_size;
		}

		/* make it easier for the vacuum process */
		memzero(&entry->data[blk_offset], sizeof(entry->data) - blk_offset);
		for (size_t i = blk_offset / row_new_size; i < (DATABLOCK_PAGE_SIZE / row_new_size); i++) {
			row = (struct row*)&entry->data[i * row_new_size];
			row->flags.deleted = false;
			row->flags.empty = true;
		}
	}

	/* update reference to the next free row within the last datablock */
	table->free_dtbkl_offset = blk_offset;
}

bool table_rem_column(struct table *table, struct column *column)
{
	int pos;
	bool found = false;

	/* sanity checks */
	if (!table || !column || table->column_count == 0)
		return false;

	/*
	 * check column name rules. Not so much because we care but
	 * if this returns false then it's sure as hell that the column
	 * doesn't exist so we can fail-fast
	 */
	if (!table_validate_column(column))
		return false;

	if (pthread_mutex_lock(&table->mutex))
		return false;

	for (pos = 0; pos < table->column_count; pos++) {
		if (strncmp(table->columns[pos].name, column->name, TABLE_MAX_COLUMN_NAME) == 0) {
			found = true;
			break;
		}
	}

	if (!found)
		goto err_cleanup_mutex;

	/* if table isn't empty than we can to rearrange rows in datablocks */
	if (!list_is_empty(table->datablock_head))
		datablock_rem_column(table, pos);

	memmove(&table->columns[pos],
		&table->columns[pos + 1],
		(TABLE_MAX_COLUMNS - (pos + 1)) * sizeof(struct column));

	table->column_count--;

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

bool table_check_var_column(struct column *column)
{
	return column->type == VARCHAR;
}

size_t table_calc_column_space(struct column *column)
{
	BUG_ON(!column);
	/* for variable length column we store a pointer to the data */
	if (table_check_var_column(column))
		return sizeof(uintptr_t);
	else
		return (size_t)column->precision;
}
