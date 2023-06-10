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

enum query_output_status {
	/* used for SELECT */
	ST_OK_WITH_RESULTS,
	/* used for CREATE, INSERT, DELETE, UPDATE */
	ST_OK_EXECUTED,
	/* used for when query couldn't be executed */
	ST_ERROR
};

struct result_set {
	int n_rows;
	int n_cols;
	char *data;
};

struct query_output_error {
	char message[1024];
};

struct query_output {
	enum query_output_status status;
	struct result_set *results;
	struct query_output_error error;
};

struct query_output* query_execute(struct database *db, char *query);

#endif /* INCLUDE_PARSER_QUERY_H_ */
