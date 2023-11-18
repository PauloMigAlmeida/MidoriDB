/*
 * query.h
 *
 *  Created on: 7/06/2023
 *      Author: paulo
 */

#ifndef INCLUDE_PARSER_QUERY_H_
#define INCLUDE_PARSER_QUERY_H_

#include <compiler/common.h>
#include <engine/database.h>
#include <primitive/table.h>

enum query_output_status {
	/* used for SELECT */
	ST_OK_WITH_RESULTS,
	/* used for CREATE, INSERT, DELETE, UPDATE */
	ST_OK_EXECUTED,
	/* used for when query couldn't be executed */
	ST_ERROR
};

struct result_set {
	struct table *table;
	struct datablock *cursor_blk;
	size_t cursor_offset;
};

struct query_output_error {
	char message[1024];
};

struct query_output {
	enum query_output_status status;
	struct result_set results;
	struct query_output_error error;
	/* number of rows affected */
	size_t n_rows_aff;
};

struct query_output* query_execute(struct database *db, char *query);

/**
 * query_cur_step - iterate through result set
 *
 * @res: result_set reference
 *
 * Returns: MIDORIDB_OK (0) if end of result_set is reached, MIDORIDB_ROW (4) if next row is available
 */
int query_cur_step(struct result_set *res);

/**
 * query_column_int64 - return information about a single column of the current result row of a query
 *
 * @res: result_set reference
 * @col_idx: column index
 *
 * Returns: int value stored at the current row given the column index
 */
int64_t query_column_int64(struct result_set *res, int col_idx);


/**
 * query_free - free resources alloc'ed when running queries against the database
 *
 * @output: query_output reference
 */
void query_free(struct query_output* output);

#endif /* INCLUDE_PARSER_QUERY_H_ */
