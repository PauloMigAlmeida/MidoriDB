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
	AST_DEL_EXPR_VAL_NAME, // column name
};

static struct ast_del_isxin_node* build_expr_isxin_node(struct queue *parser, struct stack *tmp_st, bool negation)
{
	struct ast_del_isxin_node *node;
	struct ast_node *tmp_node;
	struct stack reg_pars = {0};
	char *reg_exp;
	char *str;
	int count;

	if (!stack_init(&reg_pars))
		goto err;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_DEL_EXPRISXIN;
	node->is_negation = negation;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	str = (char*)queue_poll(parser);

	if (negation)
		reg_exp = "ISNOTIN ([0-9]+)";
	else
		reg_exp = "ISIN ([0-9]+)";

	if (!regex_ext_match_grp(str, reg_exp, &reg_pars))
		goto err_regex;

	count = atoi((char*)stack_peek_pos(&reg_pars, 0));

	/* values */
	for (int i = 0; i < count; i++) {
		tmp_node = (struct ast_node*)stack_pop(tmp_st);
		list_add(&tmp_node->head, node->node_children_head);
	}

	/* field */
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

static struct ast_del_isxnull_node* build_expr_isxnull_node(struct queue *parse, struct stack *tmp_st, bool negation)
{
	struct ast_del_isxnull_node *node;
	struct ast_node *tmp_node;

	/* discard entry */
	free(queue_poll(parse));

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_DEL_EXPRISXNULL;
	node->is_negation = negation;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	/* field */
	tmp_node = (struct ast_node*)stack_pop(tmp_st);
	list_add(&tmp_node->head, node->node_children_head);

	return node;

err_head:
	free(node);
err_node:
	return NULL;
}

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
	} else if (type == AST_DEL_EXPR_VAL_NAME) {
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

	if (type != AST_DEL_EXPR_VAL_NULL) {

		if (!regex_ext_match_grp(str, reg_exp, &reg_pars))
			goto err_regex;

		ext_val = (char*)stack_peek_pos(&reg_pars, 0);
		if (type == AST_DEL_EXPR_VAL_INTNUM) {
			node->int_val = atoi(ext_val);
		} else if (type == AST_DEL_EXPR_VAL_STRING) {
			strncpy(node->str_val, ext_val, sizeof(node->str_val) - 1);
		} else if (type == AST_DEL_EXPR_VAL_NAME) {
			strncpy(node->name_val, ext_val, sizeof(node->name_val) - 1);
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

static struct ast_del_cmp_node* build_cmp_node(struct queue *parser, struct stack *tmp_st)
{
	struct ast_del_cmp_node *node;
	struct ast_node *lhs;
	struct ast_node *rhs;
	struct stack reg_pars = {0};
	char *str;

	if (!stack_init(&reg_pars))
		goto err;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_DEL_CMP;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	str = (char*)queue_poll(parser);

	if (!regex_ext_match_grp(str, "CMP ([0-9]+)", &reg_pars))
		goto err_regex;

	node->cmp_type = atoi((char*)stack_peek_pos(&reg_pars, 0));

	rhs = (struct ast_node*)stack_pop(tmp_st);
	lhs = (struct ast_node*)stack_pop(tmp_st);

	/* from now onwards, it's node's responsibility to free what was popped out of the stack. enjoy :-)*/
	list_add(&rhs->head, node->node_children_head);
	list_add(&lhs->head, node->node_children_head);

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

static struct ast_del_logop_node* build_logop_node(struct queue *parser, struct stack *tmp_st, enum ast_logop_type logop_type)
{
	struct ast_del_logop_node *node;
	struct ast_node *condition1;
	struct ast_node *condition2;

	/* discard entry from parser */
	free(queue_poll(parser));

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_DEL_LOGOP;
	node->logop_type = logop_type;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	condition1 = (struct ast_node*)stack_pop(tmp_st);
	condition2 = (struct ast_node*)stack_pop(tmp_st);

	/* from now onwards, it's node's responsibility to free what was popped out of the stack. enjoy :-)*/
	list_add(&condition1->head, node->node_children_head);
	list_add(&condition2->head, node->node_children_head);

	return node;

err_head:
	free(node);
err_node:
	return NULL;

}

static struct ast_del_deleteone_node* build_deleteone_node(struct queue *parser, struct stack *tmp_st)
{
	struct ast_del_deleteone_node *node;
	struct ast_node *where_node;
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

	if (!stack_empty(tmp_st)) {
		/* sanity check, it means we went beyond the boundaries of this statement */
		BUG_ON(strstarts((char* )stack_peek(tmp_st), "STMT"));

		/* from now onwards, it's node's responsibility to free what was popped out of the stack. enjoy :-)*/
		where_node = (struct ast_node*)stack_pop(tmp_st);
		list_add(&where_node->head, node->node_children_head);
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

		if (strstarts(str, "NAME")) {
			curr = (struct ast_node*)build_expr_val_node(parser, AST_DEL_EXPR_VAL_NAME);
		} else if (strstarts(str, "NUMBER")) {
			curr = (struct ast_node*)build_expr_val_node(parser, AST_DEL_EXPR_VAL_INTNUM);
		} else if (strstarts(str, "STRING")) {
			curr = (struct ast_node*)build_expr_val_node(parser, AST_DEL_EXPR_VAL_STRING);
		} else if (strstarts(str, "FLOAT")) {
			curr = (struct ast_node*)build_expr_val_node(parser, AST_DEL_EXPR_VAL_APPROXNUM);
		} else if (strstarts(str, "BOOL")) {
			curr = (struct ast_node*)build_expr_val_node(parser, AST_DEL_EXPR_VAL_BOOL);
		} else if (strstarts(str, "NULL")) {
			curr = (struct ast_node*)build_expr_val_node(parser, AST_DEL_EXPR_VAL_NULL);
		} else if (strstarts(str, "CMP")) {
			curr = (struct ast_node*)build_cmp_node(parser, &st);
		} else if (strstarts(str, "AND")) {
			curr = (struct ast_node*)build_logop_node(parser, &st, AST_LOGOP_TYPE_AND);
		} else if (strstarts(str, "OR")) {
			curr = (struct ast_node*)build_logop_node(parser, &st, AST_LOGOP_TYPE_OR);
		} else if (strstarts(str, "XOR")) {
			curr = (struct ast_node*)build_logop_node(parser, &st, AST_LOGOP_TYPE_XOR);
		} else if (strstarts(str, "ISNULL")) {
			curr = (struct ast_node*)build_expr_isxnull_node(parser, &st, false);
		} else if (strstarts(str, "ISNOTNULL")) {
			curr = (struct ast_node*)build_expr_isxnull_node(parser, &st, true);
		} else if (strstarts(str, "ISIN")) {
			curr = (struct ast_node*)build_expr_isxin_node(parser, &st, false);
		} else if (strstarts(str, "ISNOTIN")) {
			curr = (struct ast_node*)build_expr_isxin_node(parser, &st, true);
		} else if (strstarts(str, "WHERE")) {
			/* "WHERE" entry doesn't have any value for AST tree, so discard it */
			free(queue_poll(parser));
			continue;
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
