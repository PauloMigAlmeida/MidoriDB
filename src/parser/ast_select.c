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

static struct ast_sel_exprop_node* build_expr_op_node(struct queue *parser, struct stack *tmp_st, enum ast_sel_expr_op_type type)
{
	struct ast_sel_exprop_node *node;
	struct ast_node *operand1;
	struct ast_node *operand2;

	/* discard entry */
	free(queue_poll(parser));

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_SEL_EXPROP;
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

static struct ast_sel_exprop_node* build_expr_neg_node(struct queue *parser, struct stack *tmp_st)
{
	struct ast_sel_exprop_node *node;
	struct ast_node *operand1;
	struct ast_sel_exprval_node *operand2;

	/* discard entry */
	free(queue_poll(parser));

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_SEL_EXPROP;
	node->op_type = AST_SEL_EXPR_OP_MUL;

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

	operand2->node_type = AST_TYPE_SEL_EXPRVAL;
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

static struct ast_sel_selectall_node* build_selectall_node(struct queue *parser)
{
	struct ast_sel_selectall_node *node;

	/* discard entry */
	free(queue_poll(parser));

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_SEL_SELECTALL;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	return node;

err_head:
	free(node);
err_node:
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

static struct ast_sel_cmp_node* build_cmp_node(struct queue *parser, struct stack *tmp_st)
{
	struct ast_sel_cmp_node *node;
	struct ast_node *lhs;
	struct ast_node *rhs;
	struct stack reg_pars = {0};
	char *str;

	if (!stack_init(&reg_pars))
		goto err;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_SEL_CMP;

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

static struct ast_sel_logop_node* build_logop_node(struct queue *parser, struct stack *tmp_st, enum ast_logop_type logop_type)
{
	struct ast_sel_logop_node *node;
	struct ast_node *condition1;
	struct ast_node *condition2;

	/* discard entry from parser */
	free(queue_poll(parser));

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_SEL_LOGOP;
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

static struct ast_sel_isxnull_node* build_expr_isxnull_node(struct queue *parse, struct stack *tmp_st, bool negation)
{
	struct ast_sel_isxnull_node *node;
	struct ast_node *tmp_node;

	/* discard entry */
	free(queue_poll(parse));

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_SEL_EXPRISXNULL;
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

static struct ast_sel_isxin_node* build_expr_isxin_node(struct queue *parser, struct stack *tmp_st, bool negation)
{
	struct ast_sel_isxin_node *node;
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

	node->node_type = AST_TYPE_SEL_EXPRISXIN;
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

static struct ast_sel_count_node* build_count_node(struct queue *parser, struct stack *tmp_st, bool all)
{
	struct ast_sel_count_node *node;
	struct ast_node *tmp_node;

	/* discard entry */
	free(queue_poll(parser));

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_SEL_COUNT;
	node->all = all;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	if (!all) {
		/* field */
		tmp_node = (struct ast_node*)stack_pop(tmp_st);
		list_add(&tmp_node->head, node->node_children_head);
	}

	return node;

err_head:
	free(node);
err_node:
	return NULL;

}

static struct ast_sel_like_node* build_like_node(struct queue *parser, struct stack *tmp_st, bool negate)
{
	struct ast_sel_like_node *node;
	struct ast_node *tmp_node;

	/* discard entry */
	free(queue_poll(parser));

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_SEL_LIKE;
	node->negate = negate;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	/* field + filter */
	for (int i = 0; i < 2; i++) {
		tmp_node = (struct ast_node*)stack_pop(tmp_st);
		list_add(&tmp_node->head, node->node_children_head);
	}

	return node;

err_head:
	free(node);
err_node:
	return NULL;

}

static struct ast_sel_onexpr_node* build_onexpr_node(struct queue *parse, struct stack *tmp_st)
{
	struct ast_sel_onexpr_node *node;
	struct ast_node *tmp_node;

	/* discard entry */
	free(queue_poll(parse));

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_SEL_ONEXPR;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	tmp_node = (struct ast_node*)stack_pop(tmp_st);
	list_add(&tmp_node->head, node->node_children_head);

	return node;

err_head:
	free(node);
err_node:
	return NULL;
}

static struct ast_sel_join_node* build_join_node(struct queue *parser, struct stack *tmp_st)
{
	struct ast_sel_join_node *node;
	struct ast_node *tmp_node;
	struct stack reg_pars = {0};
	char *str;

	if (!stack_init(&reg_pars))
		goto err;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_SEL_JOIN;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	str = (char*)queue_poll(parser);

	if (!regex_ext_match_grp(str, "JOIN ([0-9]+)", &reg_pars))
		goto err_regex;

	node->join_type = atoi((char*)stack_peek_pos(&reg_pars, 0));

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	/* ONEXPR + 1 TABLE + 1 [JOIN|TABLE] */
	for (int i = 0; i < 3; i++) {
		tmp_node = (struct ast_node*)stack_pop(tmp_st);
		list_add(&tmp_node->head, node->node_children_head);
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

static struct ast_sel_where_node* build_where_node(struct queue *parse, struct stack *tmp_st)
{
	struct ast_sel_where_node *node;
	struct ast_node *tmp_node;

	/* discard entry */
	free(queue_poll(parse));

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_SEL_WHERE;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	tmp_node = (struct ast_node*)stack_pop(tmp_st);
	list_add(&tmp_node->head, node->node_children_head);

	return node;

err_head:
	free(node);
err_node:
	return NULL;
}

static struct ast_sel_groupby_node* build_groupby_node(struct queue *parser, struct stack *tmp_st)
{
	struct ast_sel_groupby_node *node;
	struct ast_node *tmp_node;
	struct stack reg_pars = {0};
	char *str;
	int count;

	if (!stack_init(&reg_pars))
		goto err;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_SEL_GROUPBY;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	str = (char*)queue_poll(parser);

	if (!regex_ext_match_grp(str, "GROUPBYLIST ([0-9]+)", &reg_pars))
		goto err_regex;

	count = atoi((char*)stack_peek_pos(&reg_pars, 0));

	for (int i = 0; i < count; i++) {
		tmp_node = (struct ast_node*)stack_pop(tmp_st);
		list_add(&tmp_node->head, node->node_children_head);
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

static struct ast_sel_orderbylist_node* build_orderbylist_node(struct queue *parser, struct stack *tmp_st)
{
	struct ast_sel_orderbylist_node *node;
	struct ast_node *tmp_node;
	struct stack reg_pars = {0};
	char *str;
	int count;

	if (!stack_init(&reg_pars))
		goto err;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_SEL_ORDERBYLIST;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	str = (char*)queue_poll(parser);

	if (!regex_ext_match_grp(str, "ORDERBYLIST ([0-9]+)", &reg_pars))
		goto err_regex;

	count = atoi((char*)stack_peek_pos(&reg_pars, 0));

	for (int i = 0; i < count; i++) {
		tmp_node = (struct ast_node*)stack_pop(tmp_st);
		list_add(&tmp_node->head, node->node_children_head);
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

static struct ast_sel_orderbyitem_node* build_orderbyitem_node(struct queue *parser, struct stack *tmp_st)
{
	struct ast_sel_orderbyitem_node *node;
	struct ast_node *tmp_node;
	struct stack reg_pars = {0};
	char *str;

	if (!stack_init(&reg_pars))
		goto err;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_SEL_ORDERBYITEM;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	str = (char*)queue_poll(parser);

	if (!regex_ext_match_grp(str, "ORDERBYITEM ([0-9]+)", &reg_pars))
		goto err_regex;

	node->direction = atoi((char*)stack_peek_pos(&reg_pars, 0));

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

static struct ast_sel_limit_node* build_limit_node(struct queue *parser, struct stack *tmp_st)
{
	struct ast_sel_limit_node *node;
	struct ast_node *tmp_node;
	struct stack reg_pars = {0};
	char *str;
	int count;

	if (!stack_init(&reg_pars))
		goto err;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_SEL_LIMIT;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	str = (char*)queue_poll(parser);

	if (!regex_ext_match_grp(str, "LIMIT ([0-9]+)", &reg_pars))
		goto err_regex;

	count = atoi((char*)stack_peek_pos(&reg_pars, 0));

	for (int i = 0; i < count; i++) {
		tmp_node = (struct ast_node*)stack_pop(tmp_st);
		list_add(&tmp_node->head, node->node_children_head);
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

static struct ast_sel_having_node* build_having_node(struct queue *parse, struct stack *tmp_st)
{
	struct ast_sel_having_node *node;
	struct ast_node *tmp_node;

	/* discard entry */
	free(queue_poll(parse));

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_SEL_HAVING;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	tmp_node = (struct ast_node*)stack_pop(tmp_st);
	list_add(&tmp_node->head, node->node_children_head);

	return node;

err_head:
	free(node);
err_node:
	return NULL;
}

static struct ast_sel_select_node* build_select_node(struct queue *parser, struct stack *tmp_st)
{
	struct ast_sel_select_node *node;
	struct ast_node *tmp_node;
	struct stack reg_pars = {0};
	char *str;
	int count;

	if (!stack_init(&reg_pars))
		goto err;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_SEL_SELECT;

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(&node->head);
	list_head_init(node->node_children_head);

	str = (char*)queue_poll(parser);

	if (!regex_ext_match_grp(str, "SELECT ([0-9]+) ([0-9]+)", &reg_pars))
		goto err_regex;

	node->distinct = atoi((char*)stack_peek_pos(&reg_pars, 0));

	/* field count + (table_references OR joins) + where + group by + having + order by + limit */
	count = atoi((char*)stack_peek_pos(&reg_pars, 1));

	for (int i = 0; i < count; i++) {
		tmp_node = (struct ast_node*)stack_pop(tmp_st);
		list_add(&tmp_node->head, node->node_children_head);
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
		} else if (strstarts(str, "ADD")) {
			curr = (struct ast_node*)build_expr_op_node(parser, &st, AST_SEL_EXPR_OP_ADD);
		} else if (strstarts(str, "SUB")) {
			curr = (struct ast_node*)build_expr_op_node(parser, &st, AST_SEL_EXPR_OP_SUB);
		} else if (strstarts(str, "DIV")) {
			curr = (struct ast_node*)build_expr_op_node(parser, &st, AST_SEL_EXPR_OP_DIV);
		} else if (strstarts(str, "MUL")) {
			curr = (struct ast_node*)build_expr_op_node(parser, &st, AST_SEL_EXPR_OP_MUL);
		} else if (strstarts(str, "MOD")) {
			curr = (struct ast_node*)build_expr_op_node(parser, &st, AST_SEL_EXPR_OP_MOD);
		} else if (strstarts(str, "NEG")) {
			curr = (struct ast_node*)build_expr_neg_node(parser, &st);
		} else if (strstarts(str, "ALIAS")) {
			curr = (struct ast_node*)build_alias_node(parser, &st);
		} else if (strstarts(str, "FIELDNAME")) {
			curr = (struct ast_node*)build_fieldname_node(parser);
		} else if (strstarts(str, "SELECTALL")) {
			curr = (struct ast_node*)build_selectall_node(parser);
		} else if (strstarts(str, "TABLE")) {
			curr = (struct ast_node*)build_table_node(parser);
		} else if (strstarts(str, "GROUPBYLIST")) {
			curr = (struct ast_node*)build_groupby_node(parser, &st);
		} else if (strstarts(str, "ORDERBYLIST")) {
			curr = (struct ast_node*)build_orderbylist_node(parser, &st);
		} else if (strstarts(str, "ORDERBYITEM")) {
			curr = (struct ast_node*)build_orderbyitem_node(parser, &st);
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
		} else if (strstarts(str, "COUNTFIELD")) {
			curr = (struct ast_node*)build_count_node(parser, &st, false);
		} else if (strstarts(str, "COUNTALL")) {
			curr = (struct ast_node*)build_count_node(parser, &st, true);
		} else if (strstarts(str, "LIKE")) {
			curr = (struct ast_node*)build_like_node(parser, &st, false);
		} else if (strstarts(str, "NOTLIKE")) {
			curr = (struct ast_node*)build_like_node(parser, &st, true);
		} else if (strstarts(str, "ONEXPR")) {
			curr = (struct ast_node*)build_onexpr_node(parser, &st);
		} else if (strstarts(str, "JOIN")) {
			curr = (struct ast_node*)build_join_node(parser, &st);
		} else if (strstarts(str, "WHERE")) {
			curr = (struct ast_node*)build_where_node(parser, &st);
		} else if (strstarts(str, "HAVING")) {
			curr = (struct ast_node*)build_having_node(parser, &st);
		} else if (strstarts(str, "LIMIT")) {
			curr = (struct ast_node*)build_limit_node(parser, &st);
		} else if (strstarts(str, "SELECT")) {
			curr = (struct ast_node*)build_select_node(parser, &st);
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
