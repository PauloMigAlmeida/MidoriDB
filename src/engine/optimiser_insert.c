/*
 * optimiser_insert.c
 *
 *  Created on: 22/08/2023
 *      Author: paulo
 */

#include <engine/optimiser.h>

int optimiser_run_insertvals_stmt(struct database *db, struct ast_ins_insvals_node *node, struct query_output *output)
{
	UNUSED(db);
	UNUSED(node);
	UNUSED(output);

	//TODO implement Math expressions resolving optimisation (if any).
	return MIDORIDB_OK;
}
