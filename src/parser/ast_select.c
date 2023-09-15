/*
 * ast_select.c
 *
 *  Created on: 15/09/2023
 *      Author: paulo
 */

#include <parser/ast.h>
#include <lib/string.h>
#include <lib/regex.h>
#include <datastructure/stack.h>

enum ast_sel_expr_val_type {
	AST_SEL_EXPR_VAL_INTNUM,
	AST_SEL_EXPR_VAL_STRING,
	AST_SEL_EXPR_VAL_APPROXNUM,
	AST_SEL_EXPR_VAL_BOOL,
	AST_SEL_EXPR_VAL_NULL,
	AST_SEL_EXPR_VAL_NAME, // alias
};

static struct ast_sel_exprval_node* build_expr_val_node(struct queue *parser, enum ast_sel_expr_val_type type)
{
	struct ast_sel_exprval_node *node;
	struct stack reg_pars = {0};
	char *reg_exp;
	char *str;
	char *ext_val;

	if (!stack_init(&reg_pars))
		goto err;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_SEL_EXPRVAL;

	if (type == AST_SEL_EXPR_VAL_INTNUM) {
		node->value_type.is_intnum = true;
		reg_exp = "NUMBER (-{0,1}[0-9]+)";
	} else if (type == AST_SEL_EXPR_VAL_STRING) {
		node->value_type.is_str = true;
		reg_exp = "STRING '(.+)'";
	} else if (type == AST_SEL_EXPR_VAL_APPROXNUM) {
		node->value_type.is_approxnum = true;
		reg_exp = "FLOAT (-{0,1}[0-9\\.]+)";
	} else if (type == AST_SEL_EXPR_VAL_BOOL) {
		node->value_type.is_bool = true;
		reg_exp = "BOOL ([0-1])";
	} else if (type == AST_SEL_EXPR_VAL_NULL) {
		node->value_type.is_null = true;
	} else if (type == AST_SEL_EXPR_VAL_NAME) {
		node->value_type.is_name = true;
		reg_exp = "NAME (.+)";
	} else {
		die("handler not implemented for type: %d\n", type);
	}

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	str = (char*)queue_poll(parser);

	if (type != AST_SEL_EXPR_VAL_NULL) {

		if (!regex_ext_match_grp(str, reg_exp, &reg_pars))
			goto err_regex;

		ext_val = (char*)stack_peek_pos(&reg_pars, 0);
		if (type == AST_SEL_EXPR_VAL_INTNUM) {
			node->int_val = atoi(ext_val);
		} else if (type == AST_SEL_EXPR_VAL_STRING) {
			strncpy(node->str_val, ext_val, sizeof(node->str_val) - 1);
		} else if (type == AST_SEL_EXPR_VAL_NAME) {
			strncpy(node->name_val, ext_val, sizeof(node->name_val) - 1);
		} else if (type == AST_SEL_EXPR_VAL_APPROXNUM) {
			node->double_val = atof(ext_val);
		} else if (type == AST_SEL_EXPR_VAL_BOOL) {
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

static struct ast_sel_alias_node* build_alias_node(struct queue *parser, struct stack *tmp_st)
{
	struct ast_sel_alias_node *node;
	struct ast_node *tmp_node;
	struct stack reg_pars = {0};
	char *str;

	if (!stack_init(&reg_pars))
		goto err;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_SEL_ALIAS;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	str = (char*)queue_poll(parser);

	if (!regex_ext_match_grp(str, "ALIAS ([A-Za-z0-9_]+)", &reg_pars))
		goto err_regex;

	strncpy(node->alias_value, (char*)stack_peek_pos(&reg_pars, 0), sizeof(node->alias_value) - 1);

	/* field name or table */
	tmp_node = (struct ast_node*)stack_pop(tmp_st);
	list_add(&tmp_node->head, node->node_children_head);

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

static struct ast_sel_fieldname_node* build_fieldname_node(struct queue *parser)
{
	struct ast_sel_fieldname_node *node;
	struct stack reg_pars = {0};
	char *str;

	if (!stack_init(&reg_pars))
		goto err;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_SEL_FIELDNAME;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	str = (char*)queue_poll(parser);

	if (!regex_ext_match_grp(str, "FIELDNAME ([A-Za-z0-9_]+)\\.([A-Za-z0-9_]+)", &reg_pars))
		goto err_regex;

	strncpy(node->table_name, (char*)stack_peek_pos(&reg_pars, 0), sizeof(node->table_name) - 1);
	strncpy(node->col_name, (char*)stack_peek_pos(&reg_pars, 1), sizeof(node->col_name) - 1);

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

static struct ast_sel_table_node* build_table_node(struct queue *parser)
{
	struct ast_sel_table_node *node;
	struct stack reg_pars = {0};
	char *str;

	if (!stack_init(&reg_pars))
		goto err;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_SEL_TABLE;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	str = (char*)queue_poll(parser);

	if (!regex_ext_match_grp(str, "TABLE ([A-Za-z0-9_]+)", &reg_pars))
		goto err_regex;

	strncpy(node->table_name, (char*)stack_peek_pos(&reg_pars, 0), sizeof(node->table_name) - 1);

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

struct ast_node* ast_select_build_tree(struct queue *parser)
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

		if (strstarts(str, "NAME")) {
			curr = (struct ast_node*)build_expr_val_node(parser, AST_SEL_EXPR_VAL_NAME);
		} else if (strstarts(str, "NUMBER")) {
			curr = (struct ast_node*)build_expr_val_node(parser, AST_SEL_EXPR_VAL_INTNUM);
		} else if (strstarts(str, "STRING")) {
			curr = (struct ast_node*)build_expr_val_node(parser, AST_SEL_EXPR_VAL_STRING);
		} else if (strstarts(str, "FLOAT")) {
			curr = (struct ast_node*)build_expr_val_node(parser, AST_SEL_EXPR_VAL_APPROXNUM);
		} else if (strstarts(str, "BOOL")) {
			curr = (struct ast_node*)build_expr_val_node(parser, AST_SEL_EXPR_VAL_BOOL);
		} else if (strstarts(str, "NULL")) {
			curr = (struct ast_node*)build_expr_val_node(parser, AST_SEL_EXPR_VAL_NULL);
		} else if (strstarts(str, "ALIAS")) {
			curr = (struct ast_node*)build_alias_node(parser, &st);
		} else if (strstarts(str, "FIELDNAME")) {
			curr = (struct ast_node*)build_fieldname_node(parser);
		} else if (strstarts(str, "TABLE")) {
			curr = (struct ast_node*)build_table_node(parser);
		}
		//		else if (strstarts(str, "CMP")) {
//			curr = (struct ast_node*)build_cmp_node(parser, &st);
//		} else if (strstarts(str, "AND")) {
//			curr = (struct ast_node*)build_logop_node(parser, &st, AST_LOGOP_TYPE_AND);
//		} else if (strstarts(str, "OR")) {
//			curr = (struct ast_node*)build_logop_node(parser, &st, AST_LOGOP_TYPE_OR);
//		} else if (strstarts(str, "XOR")) {
//			curr = (struct ast_node*)build_logop_node(parser, &st, AST_LOGOP_TYPE_XOR);
//		} else if (strstarts(str, "ISNULL")) {
//			curr = (struct ast_node*)build_expr_isxnull_node(parser, &st, false);
//		} else if (strstarts(str, "ISNOTNULL")) {
//			curr = (struct ast_node*)build_expr_isxnull_node(parser, &st, true);
//		} else if (strstarts(str, "ISIN")) {
//			curr = (struct ast_node*)build_expr_isxin_node(parser, &st, false);
//		} else if (strstarts(str, "ISNOTIN")) {
//			curr = (struct ast_node*)build_expr_isxin_node(parser, &st, true);
//		} else if (strstarts(str, "ASSIGN")) {
//			curr = (struct ast_node*)build_assign_node(parser, &st);
//		} else if (strstarts(str, "WHERE")) {
//			/* "WHERE" entry doesn't have any value for AST tree, so discard it */
//			free(queue_poll(parser));
//			continue;
//		} else if (strstarts(str, "UPDATE")) {
//			curr = (struct ast_node*)build_update_node(parser, &st);
//		}

		else if (strstarts(str, "STMT")) {
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
