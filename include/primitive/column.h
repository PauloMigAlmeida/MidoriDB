/*
 * column.h
 *
 *  Created on: 10/04/2023
 *      Author: paulo
 */

#ifndef INCLUDE_PRIMITIVE_COLUMN_H_
#define INCLUDE_PRIMITIVE_COLUMN_H_

#include <compiler/common.h>
#include <primitive/table.h>

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
 * table_check_var_column - checks whether column has variable precision.
 *
 * @column: column reference
 *
 * This function returns true if column has variable precision, false otherwise
 */
bool table_check_var_column(struct column *column);

/**
 * table_calc_column_space - calculate how much space a given column takes up within the row
 *
 * @column: column reference
 */
size_t table_calc_column_space(struct column *column);

#endif /* INCLUDE_PRIMITIVE_COLUMN_H_ */
