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

/**
 * table_insert_row - insert row into a table
 *
 * @table: table reference
 * @row: struct row pointer to be inserted into table
 * @len: size of row data to be read from row ptr
 */
bool table_insert_row(struct table *table, struct row *row, size_t len);

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
 * @row: row's content
 * @len: size of data to be read from data ptr
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

#endif /* INCLUDE_PRIMITIVE_ROW_H_ */
