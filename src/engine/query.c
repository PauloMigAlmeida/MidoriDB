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
#include <engine/executor.h>

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

//TODO add tests
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

	if (!queue_init(&queue))
		goto err_init_queue;

	/* syntax analysis */
	if (syntax_parse(query, &queue)) {
		/* copy error from the tail of the queue */
		char *err_msg = (char*)queue_peek(&queue);
		size_t err_msg_len = sizeof(output->error.message) - 1;
		strncpy(output->error.message, err_msg, err_msg_len);
		output->status = ST_ERROR;

		/* house keeping */
		queue_free(&queue);

		return output;
	}

	/* build AST for the query */
	node = ast_build_tree(&queue);
	if (!node)
		goto err_build_ast;

	/* semantic analysis */
	if (!semantic_analyse(db, node, output->error.message, sizeof(output->error.message))) {
		/* house keeping */
		ast_free(node);
		queue_free(&queue);

		return output;
	}

	/* optimisation */
	//TODO add optimisation
	/* execution */
	executor_run(db, node, output);
	/* clean up */
	queue_free(&queue);
	ast_free(node);

	return output;

err_build_ast:
	queue_free(&queue);
err_init_queue:
	free(output);
err:
	return NULL;
}

