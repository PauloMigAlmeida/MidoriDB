/*
 * optimiser.c
 *
 *  Created on: 22/08/2023
 *      Author: paulo
 */

#include <engine/optimiser.h>

/*
 * Notes to myself:
 * 	- if there are optimisations to be made then it's OPTMISER's responsibility to
 * 	  _free_AST nodes that get replaced/discarded...
 */

int optimiser_run(struct database *db, struct ast_node *node, struct query_output *output)
{
	/* sanity checks */
	BUG_ON(!db || !node || !output);

	if (node->node_type == AST_TYPE_CRT_CREATE)
		/* there is nothing to be optimised in CREATE statements */
		return MIDORIDB_OK;
	else if (node->node_type == AST_TYPE_INS_INSVALS)
		return optimiser_run_insertvals_stmt(db, (struct ast_ins_insvals_node*)node, output);
	else if (node->node_type == AST_TYPE_DEL_DELETEONE)
		/* TODO: Delete statements can be optimised to avoid full table scans */
		return MIDORIDB_OK;
	else if (node->node_type == AST_TYPE_UPD_UPDATE)
		/* TODO: UPDATE statements can be optimised to avoid full table scans */
		return MIDORIDB_OK;
	else if (node->node_type == AST_TYPE_SEL_SELECT)
		return optimiser_run_select_stmt(db, (struct ast_sel_select_node*)node, output);
	else
		BUG_GENERIC();

	return -MIDORIDB_INTERNAL;
}
