/*
 * optimiser_insert.c
 *
 *  Created on: 22/08/2023
 *      Author: paulo
 */

#include <engine/optimiser.h>

static void handle_negation(struct ast_ins_exprval_node *in, struct ast_ins_exprval_node *out)
{
	if (in->value_type.is_intnum) {
		out->int_val = in->int_val * -1;
	} else if (in->value_type.is_approxnum) {
		out->double_val = in->double_val * -1;
	} else {
		BUG_ON_CUSTOM_MSG(true, "Operation not implemented yet\n");
	}
	memcpy(&out->value_type, &in->value_type, sizeof(out->value_type));
}

static void calcd(struct ast_ins_exprop_node *op, struct ast_ins_exprval_node *val_1,
		struct ast_ins_exprval_node *val_2, struct ast_ins_exprval_node *out)
{
	double result = 0;

	if (val_1->value_type.is_null || val_2->value_type.is_null) {
		/* if any term _became_ NULL, then we bubble that up */
		out->value_type.is_approxnum = false;
		out->value_type.is_null = true;
		result = 0;
	} else if (op->op_type == AST_INS_EXPR_OP_ADD) {
		result = val_2->double_val + val_1->double_val;
	} else if (op->op_type == AST_INS_EXPR_OP_SUB) {
		result = val_2->double_val - val_1->double_val;
	} else if (op->op_type == AST_INS_EXPR_OP_MUL) {
		result = val_2->double_val * val_1->double_val;
	} else if (op->op_type == AST_INS_EXPR_OP_DIV) {
		if (val_1->double_val == 0.0) {
			/*
			 * division by 0 in SQL result to null instead of throwing an
			 * exception like in most programming languages
			 */
			out->value_type.is_approxnum = false;
			out->value_type.is_null = true;
			result = 0;
		} else {
			result = val_2->double_val / val_1->double_val;
		}
	} else if (op->op_type == AST_INS_EXPR_OP_MOD) {
		/* C doesn't support modulo operation on double data type. Replicating SQLite behaviour */
		result = (double)(((int)val_2->double_val) % ((int)val_1->double_val));
	} else {
		BUG_ON_CUSTOM_MSG(true, "Operation not implemented yet\n");
	}

	out->double_val = result;
}

static void calci(struct ast_ins_exprop_node *op, struct ast_ins_exprval_node *val_1,
		struct ast_ins_exprval_node *val_2, struct ast_ins_exprval_node *out)
{
	int result = 0;

	if (val_1->value_type.is_null || val_2->value_type.is_null) {
		/* if any term _became_ NULL, then we bubble that up */
		out->value_type.is_intnum = false;
		out->value_type.is_null = true;
		result = 0;
	} else if (op->op_type == AST_INS_EXPR_OP_ADD) {
		result = val_2->int_val + val_1->int_val;
	} else if (op->op_type == AST_INS_EXPR_OP_SUB) {
		result = val_2->int_val - val_1->int_val;
	} else if (op->op_type == AST_INS_EXPR_OP_MUL) {
		result = val_2->int_val * val_1->int_val;
	} else if (op->op_type == AST_INS_EXPR_OP_DIV) {
		if (val_1->int_val == 0) {
			/*
			 * division by 0 in SQL result to null instead of throwing an
			 * exception like in most programming languages
			 */
			out->value_type.is_intnum = false;
			out->value_type.is_null = true;
			result = 0;
		} else {
			result = val_2->int_val / val_1->int_val;
		}
	} else if (op->op_type == AST_INS_EXPR_OP_MOD) {
		result = val_2->int_val % val_1->int_val;
	} else {
		BUG_ON_CUSTOM_MSG(true, "Operation not implemented yet\n");
	}

	out->int_val = result;
}

static int resolve_math_expr(struct ast_node **node, struct query_output *output)
{
	struct list_head *pos;
	struct ast_node *tmp_entry_1 = NULL;
	struct ast_node *tmp_entry_2 = NULL;

	struct ast_ins_exprop_node *op;
	struct ast_ins_exprval_node *val_1;
	struct ast_ins_exprval_node *val_2;
	int ret = MIDORIDB_OK;

	/* sanity check */
	BUG_ON(list_length((*node)->node_children_head) != 2);

	list_for_each(pos, (*node)->node_children_head)
	{
		if (!tmp_entry_1)
			tmp_entry_1 = list_entry(pos, typeof(*tmp_entry_1), head);
		else
			tmp_entry_2 = list_entry(pos, typeof(*tmp_entry_2), head);
	}

	BUG_ON(tmp_entry_1->node_type != AST_TYPE_INS_EXPROP && tmp_entry_1->node_type != AST_TYPE_INS_EXPRVAL);
	BUG_ON(tmp_entry_2->node_type != AST_TYPE_INS_EXPROP && tmp_entry_2->node_type != AST_TYPE_INS_EXPRVAL);

	// recursive cases
	if (!list_is_empty(tmp_entry_1->node_children_head)) {

		if ((ret = resolve_math_expr(&tmp_entry_1, output)))
			return ret;
	}

	if (!list_is_empty(tmp_entry_2->node_children_head)) {

		if ((ret = resolve_math_expr(&tmp_entry_2, output)))
			return ret;
	}

	// base case -> when we get to the bottom of the math expr
	op = (typeof(op))*node;
	val_1 = (typeof(val_1))tmp_entry_1;
	val_2 = (typeof(val_2))tmp_entry_2;

	/* calculating the result of that operation */
	if (op->op_type == AST_INS_EXPR_OP_NEG) {
		handle_negation(val_2, val_1);
	} else if (val_1->value_type.is_intnum) {
		calci(op, val_1, val_2, val_1);
	} else if (val_1->value_type.is_approxnum) {
		calcd(op, val_1, val_2, val_1);
	} else {
		BUG_ON_CUSTOM_MSG(true, "Operation not implemented yet\n");
	}

	/* juggling with pointers so I don't have to alloc something new on the heap */
	list_del(&val_1->head);
	list_add(&val_1->head, op->head.prev);
	list_del(&op->head);
	ast_free(*node);

	*node = tmp_entry_1;

	return ret;
}

static int optimise_math_expr(struct ast_ins_insvals_node *node, struct query_output *output)
{
	struct list_head *pos1;
	struct list_head *pos2;
	struct list_head *tmp_pos;
	struct ast_ins_values_node *vals_entry;
	struct ast_node *tmp_entry;
	int ret = MIDORIDB_OK;

	list_for_each(pos1, node->node_children_head)
	{
		tmp_entry = list_entry(pos1, typeof(*tmp_entry), head);

		if (tmp_entry->node_type == AST_TYPE_INS_VALUES) {

			vals_entry = (typeof(vals_entry))tmp_entry;

			list_for_each_safe(pos2, tmp_pos, vals_entry->node_children_head)
			{
				tmp_entry = list_entry(pos2, typeof(*tmp_entry), head);

				if (tmp_entry->node_type == AST_TYPE_INS_EXPROP) {
					// we are in business

					if ((ret = resolve_math_expr(&tmp_entry, output)))
						return ret;

					/* if we couldn't resolve the expression, it means something went really bad */
					BUG_ON(tmp_entry->node_type != AST_TYPE_INS_EXPRVAL);

				}
			}
		}

	}

	/* either everything went smooth or there wasn't any math expression to optimise */
	return ret;
}

int optimiser_run_insertvals_stmt(struct database *db, struct ast_ins_insvals_node *node, struct query_output *output)
{
	UNUSED(db);
	int ret;

	/* try to solve recursive math expressions (if any) */
	if ((ret = optimise_math_expr(node, output)))
		return ret;

	return MIDORIDB_OK;
}
