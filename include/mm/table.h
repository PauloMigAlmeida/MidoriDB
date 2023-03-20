/*
 * table.h
 *
 *  Created on: 4/03/2023
 *      Author: paulo
 */

#ifndef INCLUDE_MM_TABLE_H_
#define INCLUDE_MM_TABLE_H_

#include "compiler/common.h"
#include "mm/datablock.h"

#define TABLE_MAX_COLUMNS	128
#define TABLE_MAX_COLUMN_NAME	127
#define TABLE_MAX_NAME		127

struct row_header {
	/* is this row empty? */
	bool empty;
	/* was this row deleted? */
	bool deleted;
};

struct row {
	struct row_header header;
	/* actual row data - aligned for x86-64 arch */
	__x86_64_align char data[];
};

enum COLUMN_TYPE {
	VARCHAR,
	INTEGER,
	DOUBLE,
	DATE,
// Add more in the future
};

struct column {
	/* NUL-terminated string */
	char name[TABLE_MAX_COLUMN_NAME + 1 /*NUL char */];

	/* type of value meant to be stored */
	enum COLUMN_TYPE type;

	/* how much space it takes up in memory */
	int precision;

	/* is this column indexed ? */
	bool indexed;
};

struct table {

	char name[TABLE_MAX_NAME + 1 /*NUL char */];

	struct column columns[TABLE_MAX_COLUMNS];
	int column_count;

	struct list_head *datablock_head;
	/* offset from the last datablock item with free space available */
	size_t free_dtbkl_offset;

	/*
	 * using mutex locks as it is POSIX.
	 * I might, in the future, use futex (linux specific)
	 * should performance needs a bit more attention.
	 * Then again creating this database is more of a
	 * curiosity exercise rather than anything else
	 */
	pthread_mutex_t mutex;
};

/**
 * table_init - initialise a table
 *
 * @name: name of the table
 *
 * This function returns struct table* or NULL if it fails to alloc memory
 */
struct table* __must_check table_init(char *name);

/**
 * table_add_column - add column to a table
 *
 * @table: table reference
 * @column: column to be added
 *
 * This function returns true if it could add the column successfully, false otherwise
 */
bool table_add_column(struct table *table, struct column *column);

/**
 * table_rem_column - remove column from a table
 *
 * @table: table reference
 * @column: column to be removed
 *
 * This function returns true if it could remove the column successfully, false otherwise
 */
bool table_rem_column(struct table *table, struct column *column);

/**
 * table_destroy - free a table
 *
 * @table: table reference
 *
 * This function returns true if it could release all resources, false otherwise
 */
bool table_destroy(struct table **table);

/**
 * table_insert_row - insert row into a table
 *
 * @table: table reference
 * @data: ptr to data to be inserted lives
 * @len: size of data to be read from data ptr
 */
bool table_insert_row(struct table *table, void *data, size_t len);

/**
 * table_delete_row - delete row from table
 *
 * @table: table reference
 * @blk: pointer to datablock where row resides
 * @offset: offset to row inside datablock
 */
bool table_delete_row(struct table *table, struct datablock *blk, size_t offset);

/**
 * table_update_row - delete row from table
 *
 * @table: table reference
 * @blk: pointer to datablock where row resides
 * @offset: offset to row inside datablock
 * @data: ptr to data to be updated lives
 * @len: size of data to be read from data ptr
 */
bool table_update_row(struct table *table, struct datablock *blk, size_t offset, void *data, size_t len);

/**
 * table_calc_row_size - calculate payload size per row for a given table.
 *
 * @table: table reference
 *
 * This function only calculates the space to be occupied by column values
 * so in other words, it doesn't take into account row header size
 */
size_t table_calc_row_size(struct table *table);

#endif /* INCLUDE_MM_TABLE_H_ */
