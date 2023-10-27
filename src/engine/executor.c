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
	else if (node->node_type == AST_TYPE_DEL_DELETEONE)
		return executor_run_deleteone_stmt(db, (struct ast_del_deleteone_node*)node, output);
	else if (node->node_type == AST_TYPE_UPD_UPDATE)
		return executor_run_update_stmt(db, (struct ast_upd_update_node*)node, output);
	else
		/* semantic analysis not implemented for that yet */
		BUG_GENERIC();

	return -MIDORIDB_INTERNAL;
}
