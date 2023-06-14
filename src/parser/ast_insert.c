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

struct ast_node* build_col_node(struct queue *parser)
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

struct ast_node* build_inscols_node(struct queue *parser, struct stack *st)
{
	return NULL;
}

struct ast_node* build_val_num_node(struct queue *parser)
{
	return NULL;
}

struct ast_node* build_val_str_node(struct queue *parser)
{
	return NULL;
}

struct ast_node* build_values_node(struct queue *parser, struct stack *st)
{
	return NULL;
}

struct ast_node* build_insvals_node(struct queue *parser, struct stack *st)
{
	return NULL;
}

struct ast_node* ast_insert_build_tree(struct queue *parser)
{
	struct ast_node *root = NULL;
	struct ast_node *curr = NULL;
	char *str = NULL;
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
			curr = (struct ast_node*)build_val_num_node(parser);
		} else if (strstarts(str, "STRING")) {
			curr = (struct ast_node*)build_val_str_node(parser);
			//TODO add other math expr types
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

