/*
 * row.h
 *
 *  Created on: 10/04/2023
 *      Author: paulo
 */

#ifndef INCLUDE_PRIMITIVE_ROW_H_
#define INCLUDE_PRIMITIVE_ROW_H_

#include <compiler/common.h>
#include <primitive/table.h>
#include <primitive/datablock.h>

struct row_header_flags {
	/* is this row empty? */
	bool empty;
	/* was this row deleted? */
	bool deleted;
};

struct row {
	struct row_header_flags flags;
	/* is value on column <bit> set to NULL ? */
	char null_bitmap[(TABLE_MAX_COLUMNS / CHAR_BIT)];
	/* actual row data - aligned for x86-64 arch */
	__x86_64_align char data[];
};

/**
 * table_insert_row - insert row into a table
 *
 * @table: table reference
 * @row: struct row pointer to be inserted into table
 * @len: size of row data to be read from row ptr
 * 
 * Note: this method is not thread-safe. It is the caller's responsibility to
 * call table_lock() before calling this method.
 */
bool table_insert_row(struct table *table, struct row *row, size_t len);

/**
 * table_delete_row - delete row from table
 *
 * @table: table reference
 * @blk: pointer to datablock where row resides
 * @offset: offset to row inside datablock
 * 
 * Note: this method is not thread-safe. It is the caller's responsibility to
 * call table_lock() before calling this method.
 */
bool table_delete_row(struct table *table, struct datablock *blk, size_t offset);

/**
 * table_update_row - delete row from table
 *
 * @table: table reference
 * @blk: pointer to datablock where row resides
 * @offset: offset to row inside datablock
 * @row: row's content
 * @len: size of data to be read from data ptr
 * 
 * Note: this method is not thread-safe. It is the caller's responsibility to
 * call table_lock() before calling this method.
 */
bool table_update_row(struct table *table, struct datablock *blk, size_t offset, struct row *row, size_t len);

/**
 * table_calc_row_data_size - calculate payload size per row for a given table.
 *
 * @table: table reference
 *
 * This function only calculates the space to be occupied by column values
 * so in other words, it doesn't take into account row header size
 */
size_t table_calc_row_data_size(struct table *table);

/**
 * table_calc_row_size - calculate row size (header + data/payload).
 *
 * @table: table reference
 */
size_t table_calc_row_size(struct table *table);

/**
 * table_free_row_content - free row's content
 *
 * @table: table reference
 * @row: row reference
 */
void table_free_row_content(struct table *table, struct row *row);

#endif /* INCLUDE_PRIMITIVE_ROW_H_ */
