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
		//size_t err_msg_len = MIN(strlen(err_msg) + 1, sizeof(output->error.message));
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
	if (!semantic_analyse(node, output->error.message, sizeof(output->error.message))) {
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

