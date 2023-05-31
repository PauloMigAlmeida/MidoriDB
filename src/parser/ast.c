/*
 * ast.c
 *
 *  Created on: 15/05/2023
 *      Author: paulo
 */

#include <parser/ast.h>

static inline bool strstarts(const char *str, const char *prefix)
{
	return strncmp(str, prefix, strlen(prefix)) == 0;
}

static bool __must_check regex_helper(const char *text, const char *exp, struct stack *out)
{
	regex_t regex;
	regmatch_t *matches;
	char *match;
	int num_matches;

	BUG_ON(regcomp(&regex, exp, REG_EXTENDED));
	num_matches = regex.re_nsub + 1;

	matches = calloc(num_matches, sizeof(*matches));
	if (!matches)
		goto err;

	if (regexec(&regex, text, num_matches, matches, 0))
		goto err_regex;

	for (int i = 1; i < num_matches; i++) {
		int start = matches[i].rm_so;
		int end = matches[i].rm_eo;
		int length = end - start;

		match = zalloc(length + 1);
		if (!match)
			goto err_regex;

		strncpy(match, text + start, length);

		if (!stack_push(out, match, length + 1))
			goto err_stack;

		printf("Match %d: %s\n", i, match);
		free(match);
	}

	/* post sanity check */
	BUG_ON((size_t )out->idx != regex.re_nsub - 1);

	regfree(&regex);
	free(matches);

	return true;

	err_stack:
	free(match);
	err_regex:
	stack_free(out);
	regfree(&regex);
	free(matches);
	err:
	return false;
}

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

static struct ast_column_def_node* __must_check create_column_ast(struct stack *parser, int *i)
{
	struct ast_column_def_node *node;
	char *str;
	struct stack tmp_st = {0};

	/* discard "STARTCOL" as it's irrelevant now */
	*i += 1;

	node = zalloc(sizeof(*node));
	if (!node)
		goto err;

	node->node_type = AST_TYPE_COLUMNDEF;
	list_head_init(&node->head);
	node->node_children_head = NULL;
	/* unless specified otherwise, columns are nullable */
	node->attr_null = true;

	for (; *i < parser->idx; *i += 1) {
		str = (char*)stack_peek_pos(parser, *i);

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

			if (!regex_helper(str, "COLUMNDEF ([0-9]+) ([A-Za-z][A-Za-z0-9_]*)", &tmp_st))
				goto err_regex;

			parse_bison_data_type((char*)stack_peek_pos(&tmp_st, 0), node);
			strncpy(node->name,
				(char*)stack_peek_pos(&tmp_st, 1),
				strlen((char*)stack_peek_pos(&tmp_st, 1)));
			stack_free(&tmp_st);
			break;
		}
	}

	return node;

	err_regex:
	stack_free(&tmp_st);
	err_stack_init:
	free(node);
	err:
	return NULL;
}

static struct ast_create_node* __must_check create_table_ast(struct stack *parser, int *i, struct stack *tmp_st)
{
	struct ast_create_node *node;
	char *str;
	struct stack reg_pars = {0};

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

	str = (char*)stack_peek_pos(parser, *i);

	if (!regex_helper(str, "CREATE ([0-9]+) ([0-9]+) ([A-Za-z][A-Za-z0-9_]*)", &reg_pars))
		goto err_regex;

	node->if_not_exists = atoi((char*)stack_peek_pos(&reg_pars, 0));
	node->column_count = atoi((char*)stack_peek_pos(&reg_pars, 1));
	strncpy(node->table_name, (char*)stack_peek_pos(&reg_pars, 2), strlen((char*)stack_peek_pos(&reg_pars, 2)));
	stack_free(&reg_pars);

	for (int i = 0; i < node->column_count; i++) {
		struct ast_column_def_node *col = (struct ast_column_def_node*)stack_pop(tmp_st);
		/* from now onwards, it's node's responsibility to free what was popped out of the stack. enjoy :-)*/
		list_add(&col->head, node->node_children_head);
	}

	return node;

	err_regex:
	free(node->node_children_head);
	err_head:
	free(node);
	err_node:
	stack_free(&reg_pars);
	err:
	return NULL;
}

struct ast_node* ast_build_tree(struct stack *parser)
{
	struct ast_node *root = NULL;
	char *str;
	struct stack st = {0};

	/* sanity checks */
	BUG_ON(!parser || stack_empty(parser));
	BUG_ON(!strstarts((char* )stack_peek(parser), "STMT"));

	root = NULL;
	if (!stack_init(&st))
		goto err_stack_init;

	for (int i = 0; i < parser->idx + 1; i++) {
		str = (char*)stack_peek_pos(parser, i);

		if (strstarts(str, "STARTCOL")) {
			struct ast_column_def_node *node = create_column_ast(parser, &i);
			if (!node)
				goto err_alloc_node;

			if (!stack_unsafe_push(&st, node)) {
				ast_free((struct ast_node*)node);
				goto err_push_node;
			}

		} else if (strstarts(str, "CREATE")) {
			struct ast_create_node *node = create_table_ast(parser, &i, &st);
			if (!node)
				goto err_alloc_node;

			if (!stack_unsafe_push(&st, node)) {
				ast_free((struct ast_node*)node);
				goto err_push_node;
			}

		} else if (strstarts(str, "STMT")) {
			/*
			 * Notes to my future self:
			 *
			 * right now I'm just parsing a single statement but should I need to parse multiple
			 * statements then this logic has to change
			 */
			root = (struct ast_node*)stack_pop(&st);
			break;
		} else {
			fprintf(stderr, "ast_build_tree: %s handler not implement yet\n", (char*)stack_peek(parser));
			exit(1);
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
	err_alloc_node:
	stack_free(&st);
	err_stack_init:
	return NULL;
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
