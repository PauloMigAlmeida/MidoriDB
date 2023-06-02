/*
 * ast.c
 *
 *  Created on: 15/05/2023
 *      Author: paulo
 */

#include <parser/ast.h>
#include <datastructure/stack.h>
#include <lib/string.h>
#include <lib/regex.h>

static void parse_bison_data_type(char *str, struct ast_column_def_node *node)
{
	int val = atoi(str);
	int type = val / 10000;
	int precision = val % 10000;

	switch (type) {
	case 4:
		case 5:
		node->type = CT_INTEGER;
		node->precision = sizeof(uint64_t);
		break;
	case 8:
		node->type = CT_DOUBLE;
		node->precision = sizeof(double);
		break;
		//TODO figure out the right sizeof for DATE and DATETIME
	case 10:
		node->type = CT_DATE;
		node->precision = sizeof(uint64_t);
		break;
	case 11:
		node->type = CT_DATETIME;
		node->precision = sizeof(uint64_t);
		break;
	case 13:
		node->type = CT_VARCHAR;
		node->precision = precision;
		break;
	}
}

static struct ast_column_def_node* __must_check create_columndef_ast(struct queue *parser)
{
	struct ast_column_def_node *node;
	char *str = NULL;
	struct stack tmp_st = {0};

	/* discard "STARTCOL" as it's irrelevant now */
	free(queue_poll(parser));

	node = zalloc(sizeof(*node));
	if (!node)
		goto err;

	node->node_type = AST_TYPE_COLUMNDEF;
	list_head_init(&node->head);
	node->node_children_head = NULL;
	/* unless specified otherwise, columns are nullable */
	node->attr_null = true;

	while (!queue_empty(parser)) {
		str = (char*)queue_poll(parser);

		if (strstarts(str, "ATTR PRIKEY")) {
			node->attr_prim_key = true;
			node->attr_null = false;
			node->attr_not_null = true;
			node->attr_uniq_key = true;
		} else if (strstarts(str, "ATTR AUTOINC")) {
			node->attr_auto_inc = true;
		} else if (strstarts(str, "ATTR UNIQUEKEY")) {
			node->attr_uniq_key = true;
		} else if (strstarts(str, "ATTR NOTNULL")) {
			node->attr_null = false;
			node->attr_not_null = true;
		} else {
			if (!stack_init(&tmp_st))
				goto err_stack_init;

			if (!regex_ext_match_grp(str, "COLUMNDEF ([0-9]+) ([A-Za-z][A-Za-z0-9_]*)", &tmp_st))
				goto err_regex;

			parse_bison_data_type((char*)stack_peek_pos(&tmp_st, 0), node);
			strncpy(node->name,
				(char*)stack_peek_pos(&tmp_st, 1),
				strlen((char*)stack_peek_pos(&tmp_st, 1)));
			stack_free(&tmp_st);
			free(str);
			break;
		}

		free(str);
		str = NULL;
	}

	return node;

	err_regex:
	stack_free(&tmp_st);
	err_stack_init:
	free(node);
	if (!str)
		free(str);
	err:
	return NULL;
}

static struct ast_create_node* __must_check create_table_ast(struct queue *parser, struct stack *tmp_st)
{
	struct ast_create_node *node;
	char *str;
	struct stack reg_pars = {0};
	int count;

	if (!stack_init(&reg_pars))
		goto err;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_CREATE;
	list_head_init(&node->head);

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(node->node_children_head);

	str = (char*)queue_poll(parser);

	if (!regex_ext_match_grp(str, "CREATE ([0-9]+) ([0-9]+) ([A-Za-z][A-Za-z0-9_]*)", &reg_pars))
		goto err_regex;

	node->if_not_exists = atoi((char*)stack_peek_pos(&reg_pars, 0));
	count = atoi((char*)stack_peek_pos(&reg_pars, 1));
	strncpy(node->table_name, (char*)stack_peek_pos(&reg_pars, 2), strlen((char*)stack_peek_pos(&reg_pars, 2)));

	for (int i = 0; i < count; i++) {
		struct ast_column_def_node *col = (struct ast_column_def_node*)stack_pop(tmp_st);
		/* from now onwards, it's node's responsibility to free what was popped out of the stack. enjoy :-)*/
		list_add(&col->head, node->node_children_head);
	}

	stack_free(&reg_pars);
	free(str);

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

static struct ast_index_column_node* __must_check create_indexcol_ast(struct queue *parser)
{
	struct ast_index_column_node *node;
	char *str = NULL;
	struct stack reg_pars = {0};

	node = zalloc(sizeof(*node));
	if (!node)
		goto err;

	node->node_type = AST_TYPE_INDEXCOL;
	list_head_init(&node->head);
	node->node_children_head = NULL;

	str = (char*)queue_poll(parser);

	if (!stack_init(&reg_pars))
		goto err_stack_init;

	if (!regex_ext_match_grp(str, "COLUMN ([A-Za-z][A-Za-z0-9_]*)", &reg_pars))
		goto err_regex;

	strncpy(node->name,
		(char*)stack_peek_pos(&reg_pars, 0),
		strlen((char*)stack_peek_pos(&reg_pars, 0)));
	stack_free(&reg_pars);
	free(str);

	return node;

	err_regex:
	stack_free(&reg_pars);
	err_stack_init:
	free(str);
	free(node);
	err:
	return NULL;
}

static struct ast_index_def_node* __must_check create_indexdef_ast(struct queue *parser, struct stack *tmp_st)
{
	struct ast_index_def_node *node;
	char *str;
	struct stack reg_pars = {0};
	int count;

	if (!stack_init(&reg_pars))
		goto err;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err_node;

	node->node_type = AST_TYPE_INDEXDEF;
	list_head_init(&node->head);

	if (!(node->node_children_head = malloc(sizeof(*node->node_children_head))))
		goto err_head;

	list_head_init(node->node_children_head);

	str = (char*)queue_poll(parser);

	if (!regex_ext_match_grp(str, "PRIKEY ([0-9]+)", &reg_pars))
		goto err_regex;

	node->is_pk = true;

	count = atoi((char*)stack_peek_pos(&reg_pars, 0));

	for (int i = 0; i < count; i++) {
		struct ast_index_column_node *col = (struct ast_index_column_node*)stack_pop(tmp_st);
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

struct ast_node* __create_build_tree(struct queue *parser)
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

		if (strstarts(str, "STARTCOL")) {
			curr = (struct ast_node*)create_columndef_ast(parser);
		} else if (strstarts(str, "CREATE")) {
			curr = (struct ast_node*)create_table_ast(parser, &st);
		} else if (strstarts(str, "COLUMN")) {
			curr = (struct ast_node*)create_indexcol_ast(parser);
		} else if (strstarts(str, "PRIKEY")) {
			curr = (struct ast_node*)create_indexdef_ast(parser, &st);
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

struct ast_node* ast_build_tree(struct queue *parser)
{
	char *str = NULL;
	size_t pos = 0;
	bool found = false;

	/* sanity checks */
	BUG_ON(!parser || queue_empty(parser));
//	BUG_ON(!strstarts((char* stack_peek(parser), "STMT")); //TODO adapt this for queue

	/* walk to STMT */
	while (pos < queue_length(parser)) {
		if (strstarts((char*)queue_peek_pos(parser, pos), "STMT")) {
			found = true;
			break;
		}
		pos++;
	}

	/* something terribly wrong if this is true */
	BUG_ON(!found);

	/*
	 * Notes to my future self:
	 *
	 * right now I'm just parsing a single statement but should I need to parse multiple
	 * statements then this logic has to change
	 */

	/* decide which routine to best parse its contents based on the operation type */
	pos--;
	str = (char*)queue_peek_pos(parser, pos);
	if (strstarts(str, "CREATE")) {
		return __create_build_tree(parser);
	} else {
		fprintf(stderr, "%s: %s handler not implement yet\n", __func__, str);
		exit(1);
	}
}

void ast_free(struct ast_node *node)
{
	struct list_head *pos = NULL;
	struct list_head *tmp_pos = NULL;
	struct ast_node *entry = NULL;

	if (node->node_children_head) {

		list_for_each_safe(pos,tmp_pos, node->node_children_head)
		{
			entry = list_entry(pos, typeof(*entry), head);
			ast_free(entry);
		}

		free(node->node_children_head);
	}

	free(node);
}
