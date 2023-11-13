/*
 * optimiser_select.c
 *
 *  Created on: 14/11/2023
 *      Author: paulo
 */

#include <engine/optimiser.h>

int optimiser_run_select_stmt(struct database *db, struct ast_sel_select_node *node, struct query_output *output)
{
	UNUSED(db);
	UNUSED(node);
	UNUSED(output);

	/*
	 * TODO: Replace every exprval (name) with a fieldname using the full-qualified column format
	 * so that executor's code becomes "less" complicated.
	 *
	 * This also implies in removing table alias as part of the process.
	 */

	return MIDORIDB_OK;
}
