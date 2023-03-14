/*
 * column.c
 *
 *  Created on: 13/03/2023
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

static bool table_validate_column(struct column *column)
{
	if (!__valid_name(column->name, TABLE_MAX_COLUMN_NAME))
		return false;

	if (column->precision < 1)
		return false;

	return true;
}

bool table_add_column(struct table *table, struct column *column)
{
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
