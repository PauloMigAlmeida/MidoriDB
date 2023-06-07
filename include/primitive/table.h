/*
 * table.h
 *
 *  Created on: 4/03/2023
 *      Author: paulo
 */

#ifndef INCLUDE_PRIMITIVE_TABLE_H_
#define INCLUDE_PRIMITIVE_TABLE_H_

#include <primitive/datablock.h>
#include <primitive/column.h>
#include <compiler/common.h>
#include <lib/bit.h>

#define TABLE_MAX_COLUMNS		128
#define TABLE_MAX_NAME			127

/* sanity checks */
BUILD_BUG(TABLE_MAX_COLUMNS > 8, "TABLE_MAX_COLUMNS has to be greater than 8")
BUILD_BUG(IS_POWER_OF_2(TABLE_MAX_COLUMNS), "TABLE_MAX_COLUMNS must be a power of 2")

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
 * table_destroy - free a table
 *
 * @table: table reference
 *
 * This function returns true if it could release all resources, false otherwise
 */
bool table_destroy(struct table **table);

/**
 * table_vacuum - perform table vaccum on existing datablocks
 *
 * @table: table reference
 */
bool table_vacuum(struct table *table);

/**
 * table_datablock_init - initialise datablock for a given table
 *
 * @table: table reference
 * @offset: row offset
 * @row_size: size of each row
 */
void table_datablock_init(struct datablock *block, size_t offset, size_t row_size);

/**
 * table_validate_name - valida name of table
 *
 * @name: name of the table
 *
 * this function returns true if name is valid, false otherwise
 */
bool table_validate_name(char *name);

#endif /* INCLUDE_PRIMITIVE_TABLE_H_ */
