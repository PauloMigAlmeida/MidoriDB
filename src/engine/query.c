/*
 * query.c
 *
 *  Created on: 7/06/2023
 *      Author: paulo
 */

#include <engine/query.h>
#include <parser/syntax.h>
#include <parser/ast.h>
#include <parser/semantic.h>
#include <datastructure/queue.h>
#include <engine/optimiser.h>
#include <engine/executor.h>
#include <primitive/row.h>
#include <primitive/column.h>

/**
 * Notes to my future self:
 * 	-> I removed the locking references from executor_*.c files as this was leading to some validations to be
 * 		performed at the execution phase that I consider to be part of the semantic phase. For instance, to
 * 		test whether the date format of a string would parse before turning it into time_t:
 *
 * 		INSERT INTO TEST VALUES ('2023-06-30 21:11:00');
 *
 * 		The reason I was initially doing it on execution phase was because I had the table lock but this was
 * 		making the source code rather messy. So I decided to implement the locking actions only when I start
 * 		meddling with the transaction piece of the database -> Which I haven't started yet.
 *
 * 		I don't have everything figured out yet but I am assuming that I will be able to lock tables as I go
 * 		through different phases (and as early as semantic phase) and only release it after the the execution
 * 		phase.... some sort of hash map of locked elements pointer that I pass around. (TBD)
 */

struct query_output* query_execute(struct database *db, char *query)
{
	struct query_output *output = NULL;
	struct queue queue = {0};
	struct ast_node *node = NULL;

	/* sanity checks */
	BUG_ON(!query);

	output = zalloc(sizeof(*output));
	if (!output)
		goto err;

	if (!queue_init(&queue)) {
		snprintf(output->error.message, sizeof(output->error.message) - 1, "error while initialising query\n");
		goto err_init_queue;
	}

	/* syntax analysis */
	if (syntax_parse(query, &queue)) {
		/* copy error from the tail of the queue */
		char *err_msg = (char*)queue_peek(&queue);
		size_t err_msg_len = sizeof(output->error.message) - 1;
		strncpy(output->error.message, err_msg, err_msg_len);

		goto err_syntax_parser;
	}

	/* build AST for the query */
	node = ast_build_tree(&queue);
	if (!node) {
		snprintf(output->error.message, sizeof(output->error.message) - 1,
				"error while running syntax analysis on query\n");
		goto err_build_ast;
	}

	/* semantic analysis */
	if (!semantic_analyse(db, node, output->error.message, sizeof(output->error.message) - 1)) {
		goto err_semantic_analysis;
	}

	/* optimisation */
	if (optimiser_run(db, node, output))
		goto err_optimisation_phase;

	/* execution */
	if (executor_run(db, node, output))
		goto err_execution_phase;

	if (node->node_type == AST_TYPE_SEL_SELECT)
		output->status = ST_OK_WITH_RESULTS;
	else
		output->status = ST_OK_EXECUTED;

	/* clean up */
	queue_free(&queue);
	ast_free(node);

	return output;

err_execution_phase:
err_optimisation_phase:
err_semantic_analysis:
	ast_free(node);
err_build_ast:
err_syntax_parser:
	queue_free(&queue);
err_init_queue:
	output->status = ST_ERROR;
err:
	return output;
}

int query_cur_step(struct result_set *res)
{
	struct row *row;
	size_t row_size;

	/* sanity checks */
	BUG_ON(!res || !res->table);

	row_size = table_calc_row_size(res->table);

	/* is this the first time ? */
	if (!res->cursor_blk) {

		res->cursor_blk = container_of(res->table->datablock_head->next, typeof(struct datablock), head);
		res->cursor_offset = 0;
	} else {
		if (res->cursor_offset + row_size > DATABLOCK_PAGE_SIZE) {
			res->cursor_blk = container_of(res->cursor_blk->head.next, typeof(struct datablock), head);
			res->cursor_offset = 0;

			if (&res->cursor_blk->head == res->table->datablock_head) {
				/* end of the line */
				return MIDORIDB_OK;
			}
		} else {
			res->cursor_offset += row_size;
		}
	}

	/* at this point we shouldn't have any of this, so clearly something went really wrong here */
	row = (typeof(row))&res->cursor_blk->data[res->cursor_offset];
	BUG_ON(row->flags.deleted);

	if (row->flags.empty)
		/* end of the line */
		return MIDORIDB_OK;

	return MIDORIDB_ROW;
}

int64_t query_column_int64(struct result_set *res, int col_idx)
{
	struct row *row;
	size_t offset = 0;
	int64_t ret = 0;

	/* sanity checks */
	BUG_ON(!res || !res->table || !res->cursor_blk || col_idx > res->table->column_count - 1);

	row = (typeof(row))&res->cursor_blk->data[res->cursor_offset];

	/* sounds like user neither invoked query_cur_step nor checked if it returned MIDORIDB_ROW */
	BUG_ON_CUSTOM_MSG(row->flags.deleted || row->flags.empty, "cursor is pointing at an invalid row\n");

	for (int i = 0; i < col_idx; i++)
		offset += table_calc_column_space(&res->table->columns[i]);

	ret = *(int64_t*)(row->data + offset);
	return ret;
}

void query_free(struct query_output* output)
{
	if (output->status == ST_OK_WITH_RESULTS){
		table_destroy(&output->results.table);
	}

	free(output);
}
