/*
 * executor.c
 *
 *  Created on: 8/06/2023
 *      Author: paulo
 */

#include <engine/executor.h>

int executor_run(struct database *db, struct ast_node *node, struct query_output *output)
{
	/* sanity checks */
	BUG_ON(!db || !node || !output);

	if (node->node_type == AST_TYPE_CRT_CREATE)
		return executor_run_create_stmt(db, (struct ast_crt_create_node*)node, output);
	else if (node->node_type == AST_TYPE_INS_INSVALS)
		return executor_run_insertvals_stmt(db, (struct ast_ins_insvals_node*)node, output);
	else
		/* semantic analysis not implemented for that yet */
		BUG_ON(true);

	return -MIDORIDB_INTERNAL;
}
