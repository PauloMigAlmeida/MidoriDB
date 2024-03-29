/*
 * ast_insert.c
 *
 *  Created on: 13/06/2023
 *      Author: paulo
 */

#include <parser/ast.h>
#include <lib/string.h>
#include <lib/regex.h>
#include <datastructure/stack.h>

enum ast_ins_expr_val_type {
	AST_INS_EXPR_VAL_INTNUM,
	AST_INS_EXPR_VAL_STRING,
	AST_INS_EXPR_VAL_APPROXNUM,
	AST_INS_EXPR_VAL_BOOL,
	AST_INS_EXPR_VAL_NULL,
};

static struct ast_ins_column_node* build_col_node(struct queue *parser)
{
	struct ast_ins_column_node *node;
	struct stack reg_pars = {0};
	char *str;

	if (!stack_init(&reg_pars))
		goto err;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_INS_COLUMN;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	str = (char*)queue_poll(parser);

	if (!regex_ext_match_grp(str, "COLUMN ([A-Za-z0-9_]+)", &reg_pars))
		goto err_regex;

	strncpy(node->name, (char*)stack_peek_pos(&reg_pars, 0), sizeof(node->name) - 1 /* NUL-char */);

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

static struct ast_ins_inscols_node* build_inscols_node(struct queue *parser, struct stack *tmp_st)
{
	struct ast_ins_inscols_node *node;
	struct stack reg_pars = {0};
	char *str;

	if (!stack_init(&reg_pars))
		goto err;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_INS_INSCOLS;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	str = (char*)queue_poll(parser);

	if (!regex_ext_match_grp(str, "INSERTCOLS ([0-9]+)", &reg_pars))
		goto err_regex;

	node->column_count = atoi((char*)stack_peek_pos(&reg_pars, 0));

	for (int i = 0; i < node->column_count; i++) {
		struct ast_ins_column_node *col = (struct ast_ins_column_node*)stack_pop(tmp_st);
		/* from now onwards, it's node's responsibility to free what was popped out of the stack. enjoy :-)*/
		list_add(&col->head, node->node_children_head);
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

static struct ast_ins_exprval_node* build_expr_val_node(struct queue *parser, enum ast_ins_expr_val_type type)
{
	struct ast_ins_exprval_node *node;
	struct stack reg_pars = {0};
	char *reg_exp;
	char *str;
	char *ext_val;

	if (!stack_init(&reg_pars))
		goto err;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_INS_EXPRVAL;

	if (type == AST_INS_EXPR_VAL_INTNUM) {
		node->value_type.is_intnum = true;
		reg_exp = "NUMBER (-{0,1}[0-9]+)";
	} else if (type == AST_INS_EXPR_VAL_STRING) {
		node->value_type.is_str = true;
		reg_exp = "STRING '(.+)'";
	} else if (type == AST_INS_EXPR_VAL_APPROXNUM) {
		node->value_type.is_approxnum = true;
		reg_exp = "FLOAT (-{0,1}[0-9\\.]+)";
	} else if (type == AST_INS_EXPR_VAL_BOOL) {
		node->value_type.is_bool = true;
		reg_exp = "BOOL ([0-1])";
	} else if (type == AST_INS_EXPR_VAL_NULL) {
		node->value_type.is_null = true;
	} else {
		die("handler not implemented for type: %d\n", type);
	}

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	str = (char*)queue_poll(parser);

	if (type != AST_INS_EXPR_VAL_NULL) {

		if (!regex_ext_match_grp(str, reg_exp, &reg_pars))
			goto err_regex;

		ext_val = (char*)stack_peek_pos(&reg_pars, 0);
		if (type == AST_INS_EXPR_VAL_INTNUM) {
			node->int_val = atoi(ext_val);
		} else if (type == AST_INS_EXPR_VAL_STRING) {
			strncpy(node->str_val, ext_val, sizeof(node->str_val) - 1);
		} else if (type == AST_INS_EXPR_VAL_APPROXNUM) {
			node->double_val = atof(ext_val);
		} else if (type == AST_INS_EXPR_VAL_BOOL) {
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

static struct ast_ins_exprop_node* build_expr_op_node(struct queue *parser, struct stack *tmp_st, enum ast_ins_expr_op_type type)
{
	struct ast_ins_exprop_node *node;
	struct ast_node *operand1;
	struct ast_node *operand2;

	/* discard entry */
	free(queue_poll(parser));

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_INS_EXPROP;
	node->op_type = type;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	operand2 = (struct ast_node*)stack_pop(tmp_st);
	operand1 = (struct ast_node*)stack_pop(tmp_st);

	list_add(&operand1->head, node->node_children_head);
	list_add(&operand2->head, node->node_children_head);

	return node;

err_head:
	free(node);
err_node:
	return NULL;
}

static struct ast_ins_exprop_node* build_expr_neg_node(struct queue *parser, struct stack *tmp_st)
{
	struct ast_ins_exprop_node *node;
	struct ast_node *operand1;
	struct ast_ins_exprval_node *operand2;

	/* discard entry */
	free(queue_poll(parser));

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_INS_EXPROP;
	node->op_type = AST_INS_EXPR_OP_MUL;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	operand2 = zalloc(sizeof(*operand2));
	if (!operand2)
		goto err_operand1;

	if (!(operand2->node_children_head = malloc(sizeof(*operand2->node_children_head))))
		goto err_operand1_head;

	list_head_init(&operand2->head);
	list_head_init(operand2->node_children_head);

	operand2->node_type = AST_TYPE_INS_EXPRVAL;
	operand2->int_val = -1;
	operand2->value_type.is_negation = true;

	operand1 = (struct ast_node*)stack_pop(tmp_st);

	list_add(&operand1->head, node->node_children_head);
	list_add(&operand2->head, node->node_children_head);

	return node;

err_operand1_head:
	free(operand2);
err_operand1:
	free(node->node_children_head);
err_head:
	free(node);
err_node:
	return NULL;
}

static struct ast_ins_values_node* build_values_node(struct queue *parser, struct stack *tmp_st)
{
	struct ast_ins_values_node *node;
	struct stack reg_pars = {0};
	char *str;

	if (!stack_init(&reg_pars))
		goto err;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_INS_VALUES;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	str = (char*)queue_poll(parser);

	if (!regex_ext_match_grp(str, "VALUES ([0-9]+)", &reg_pars))
		goto err_regex;

	node->value_count = atoi((char*)stack_peek_pos(&reg_pars, 0));

	for (int i = 0; i < node->value_count; i++) {
		/* as far as we are concerned, this can be anything expr val, func, expr op... so use the generic one */
		struct ast_node *val = (struct ast_node*)stack_pop(tmp_st);
		/* from now onwards, it's node's responsibility to free what was popped out of the stack. enjoy :-)*/
		list_add(&val->head, node->node_children_head);
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

static struct ast_ins_insvals_node* build_insvals_node(struct queue *parser, struct stack *tmp_st)
{
	struct ast_ins_insvals_node *node;
	struct stack reg_pars = {0};
	char *str;

	if (!stack_init(&reg_pars))
		goto err;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_INS_INSVALS;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	str = (char*)queue_poll(parser);

	if (!regex_ext_match_grp(str, "INSERTVALS ([0-9]+) ([0-9]+) ([A-Za-z0-9_]*)", &reg_pars))
		goto err_regex;

	node->opt_column_list = atoi((char*)stack_peek_pos(&reg_pars, 0));
	node->row_count = atoi((char*)stack_peek_pos(&reg_pars, 1));
	strncpy(node->table_name, (char*)stack_peek_pos(&reg_pars, 2), sizeof(node->table_name) - 1 /* NUL-char */);

	for (int i = 0; i < node->row_count + node->opt_column_list; i++) {
		struct ast_ins_values_node *val = (struct ast_ins_values_node*)stack_pop(tmp_st);
		/* from now onwards, it's node's responsibility to free what was popped out of the stack. enjoy :-)*/
		list_add(&val->head, node->node_children_head);
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

struct ast_node* ast_insert_build_tree(struct queue *parser)
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

		if (strstarts(str, "COLUMN")) {
			curr = (struct ast_node*)build_col_node(parser);
		} else if (strstarts(str, "INSERTCOLS")) {
			curr = (struct ast_node*)build_inscols_node(parser, &st);
		} else if (strstarts(str, "NUMBER")) {
			curr = (struct ast_node*)build_expr_val_node(parser, AST_INS_EXPR_VAL_INTNUM);
		} else if (strstarts(str, "STRING")) {
			curr = (struct ast_node*)build_expr_val_node(parser, AST_INS_EXPR_VAL_STRING);
		} else if (strstarts(str, "FLOAT")) {
			curr = (struct ast_node*)build_expr_val_node(parser, AST_INS_EXPR_VAL_APPROXNUM);
		} else if (strstarts(str, "BOOL")) {
			curr = (struct ast_node*)build_expr_val_node(parser, AST_INS_EXPR_VAL_BOOL);
		} else if (strstarts(str, "NULL")) {
			curr = (struct ast_node*)build_expr_val_node(parser, AST_INS_EXPR_VAL_NULL);
		} else if (strstarts(str, "ADD")) {
			curr = (struct ast_node*)build_expr_op_node(parser, &st, AST_INS_EXPR_OP_ADD);
		} else if (strstarts(str, "SUB")) {
			curr = (struct ast_node*)build_expr_op_node(parser, &st, AST_INS_EXPR_OP_SUB);
		} else if (strstarts(str, "DIV")) {
			curr = (struct ast_node*)build_expr_op_node(parser, &st, AST_INS_EXPR_OP_DIV);
		} else if (strstarts(str, "MUL")) {
			curr = (struct ast_node*)build_expr_op_node(parser, &st, AST_INS_EXPR_OP_MUL);
		} else if (strstarts(str, "MOD")) {
			curr = (struct ast_node*)build_expr_op_node(parser, &st, AST_INS_EXPR_OP_MOD);
		} else if (strstarts(str, "NEG")) {
			curr = (struct ast_node*)build_expr_neg_node(parser, &st);
		} else if (strstarts(str, "VALUES")) {
			curr = (struct ast_node*)build_values_node(parser, &st);
		} else if (strstarts(str, "INSERTVALS")) {
			curr = (struct ast_node*)build_insvals_node(parser, &st);
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

