/*
 * semantic.c
 *
 *  Created on: 7/06/2023
 *      Author: paulo
 */

#include <parser/semantic.h>
#include <engine/database.h>
#include <parser/ast.h>

bool semantic_analyse(struct database *db, struct ast_node *node, char *out_err, size_t out_err_len)
{
	/* sanity checks */
	BUG_ON(!node || !out_err || out_err_len == 0);

	if (node->node_type == AST_TYPE_CRT_CREATE)
		return semantic_analyse_create_stmt(db, node, out_err, out_err_len);
	else if (node->node_type == AST_TYPE_INS_INSVALS)
		return semantic_analyse_insert_stmt(db, node, out_err, out_err_len);
	else if (node->node_type == AST_TYPE_DEL_DELETEONE)
		return semantic_analyse_delete_stmt(db, node, out_err, out_err_len);
	else if (node->node_type == AST_TYPE_UPD_UPDATE)
		return semantic_analyse_update_stmt(db, node, out_err, out_err_len);
	else if (node->node_type == AST_TYPE_SEL_SELECT)
		return semantic_analyse_select_stmt(db, node, out_err, out_err_len);
	else
		/* semantic analysis not implemented for that yet */
		BUG_ON(true);

	return false;
}
