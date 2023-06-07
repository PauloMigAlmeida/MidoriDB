/*
 * column.h
 *
 *  Created on: 10/04/2023
 *      Author: paulo
 */

#ifndef INCLUDE_PRIMITIVE_COLUMN_H_
#define INCLUDE_PRIMITIVE_COLUMN_H_

#include <compiler/common.h>

#define TABLE_MAX_COLUMN_NAME		127

struct table;

enum COLUMN_TYPE {
	CT_VARCHAR,
	CT_INTEGER,
	CT_DOUBLE,
	CT_DATE,
	CT_DATETIME,
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

#define TABLE_MAX_COLUMN_NAME		127

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

/*
 * table_validate_column_name - validate column name
 *
 * @name: name of the column
 *
 * this function returns true if valid, false otherwise
 */
bool table_validate_column_name(char *name);

#endif /* INCLUDE_PRIMITIVE_COLUMN_H_ */
