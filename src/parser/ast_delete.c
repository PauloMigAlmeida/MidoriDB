/*
 * ast_delete.c
 *
 *  Created on: 25/08/2023
 *      Author: paulo
 */

#include <parser/ast.h>
#include <lib/string.h>
#include <lib/regex.h>
#include <datastructure/stack.h>

enum ast_del_expr_val_type {
	AST_DEL_EXPR_VAL_INTNUM,
	AST_DEL_EXPR_VAL_STRING,
	AST_DEL_EXPR_VAL_APPROXNUM,
	AST_DEL_EXPR_VAL_BOOL,
	AST_DEL_EXPR_VAL_NULL,
};

static struct ast_del_exprval_node* build_expr_val_node(struct queue *parser, enum ast_del_expr_val_type type)
{
	struct ast_del_exprval_node *node;
	struct stack reg_pars = {0};
	char *reg_exp;
	char *str;
	char *ext_val;

	if (!stack_init(&reg_pars))
		goto err;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_DEL_EXPRVAL;

	if (type == AST_DEL_EXPR_VAL_INTNUM) {
		node->value_type.is_intnum = true;
		reg_exp = "NUMBER (-{0,1}[0-9]+)";
	} else if (type == AST_DEL_EXPR_VAL_STRING) {
		node->value_type.is_str = true;
		reg_exp = "STRING '(.+)'";
	} else if (type == AST_DEL_EXPR_VAL_APPROXNUM) {
		node->value_type.is_approxnum = true;
		reg_exp = "FLOAT (-{0,1}[0-9\\.]+)";
	} else if (type == AST_DEL_EXPR_VAL_BOOL) {
		node->value_type.is_bool = true;
		reg_exp = "BOOL ([0-1])";
	} else if (type == AST_DEL_EXPR_VAL_NULL) {
		node->value_type.is_null = true;
	} else {
		die("handler not implemented for type: %d\n", type);
	}

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	str = (char*)queue_poll(parser);

	if (type != AST_DEL_EXPR_VAL_NULL) {

		if (!regex_ext_match_grp(str, reg_exp, &reg_pars))
			goto err_regex;

		ext_val = (char*)stack_peek_pos(&reg_pars, 0);
		if (type == AST_DEL_EXPR_VAL_INTNUM) {
			node->int_val = atoi(ext_val);
		} else if (type == AST_DEL_EXPR_VAL_STRING) {
			strncpy(node->str_val, ext_val, sizeof(node->str_val) - 1);
		} else if (type == AST_DEL_EXPR_VAL_APPROXNUM) {
			node->double_val = atof(ext_val);
		} else if (type == AST_DEL_EXPR_VAL_BOOL) {
			node->bool_val = atoi(ext_val);
		} else {
			die("handler not implemented for type: %d\n", type);
		}

	}

	free(str);

	stack_free(&reg_pars);

	return node;

err_regex:
	free(str);
	free(node->node_children_head);
err_head:
	free(node);
err_node:
	stack_free(&reg_pars);
err:
	return NULL;
}

static struct ast_del_deleteone_node* build_deleteone_node(struct queue *parser, struct stack *tmp_st)
{
	struct ast_del_deleteone_node *node;
	struct stack reg_pars = {0};
	char *str;

	if (!stack_init(&reg_pars))
		goto err;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_DEL_DELETEONE;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	str = (char*)queue_poll(parser);

	if (!regex_ext_match_grp(str, "DELETEONE ([A-Za-z0-9_]*)", &reg_pars))
		goto err_regex;

	strncpy(node->table_name, (char*)stack_peek_pos(&reg_pars, 0), sizeof(node->table_name) - 1 /* NUL-char */);

//	//TODO I started from the opposite direction...so when I implement WHERE I can check if
//	// it exists and add as a node to node_children
	if (!stack_empty(tmp_st)) {
//		struct ast_del_where_node *val = (struct ast_del_where_node*)stack_pop(tmp_st);
//		/* from now onwards, it's node's responsibility to free what was popped out of the stack. enjoy :-)*/
//		list_add(&val->head, node->node_children_head);
	}

	free(str);
	stack_free(&reg_pars);

	return node;

err_regex:
	free(str);
	free(node->node_children_head);
err_head:
	free(node);
err_node:
	stack_free(&reg_pars);
err:
	return NULL;

}

struct ast_node* ast_delete_build_tree(struct queue *parser)
{
	struct ast_node *root;
	struct ast_node *curr;
	char *str;
	struct stack st = {0};

	root = NULL;
	if (!stack_init(&st))
		goto err_stack_init;

	while (!queue_empty(parser)) {
		str = (char*)queue_peek(parser);

		if (strstarts(str, "NUMBER")) {
			curr = (struct ast_node*)build_expr_val_node(parser, AST_DEL_EXPR_VAL_INTNUM);
		} else if (strstarts(str, "STRING")) {
			curr = (struct ast_node*)build_expr_val_node(parser, AST_DEL_EXPR_VAL_STRING);
		} else if (strstarts(str, "FLOAT")) {
			curr = (struct ast_node*)build_expr_val_node(parser, AST_DEL_EXPR_VAL_APPROXNUM);
		} else if (strstarts(str, "BOOL")) {
			curr = (struct ast_node*)build_expr_val_node(parser, AST_DEL_EXPR_VAL_BOOL);
		} else if (strstarts(str, "NULL")) {
			curr = (struct ast_node*)build_expr_val_node(parser, AST_DEL_EXPR_VAL_NULL);
		} else if (strstarts(str, "DELETEONE")) {
			curr = (struct ast_node*)build_deleteone_node(parser, &st);
		} else if (strstarts(str, "STMT")) {
			root = (struct ast_node*)stack_pop(&st);
			break;
		} else {
			fprintf(stderr, "%s: %s handler not implement yet\n", __func__, str);
			exit(1);
		}

		if (!curr)
			goto err_push_node;

		if (!stack_unsafe_push(&st, curr)) {
			ast_free(curr);
			goto err_push_node;
		}

	}

	/* something went terribly wrong is this is not empty */
	BUG_ON(!stack_empty(&st));
	stack_free(&st);

	return root;

err_push_node:
	while (!stack_empty(&st)) {
		ast_free((struct ast_node*)stack_pop(&st));
	}
	stack_free(&st);
err_stack_init:
	return NULL;
}
