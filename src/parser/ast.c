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

	return false;

	err_stack:
	free(match);
	err_regex:
	stack_free(out);
	regfree(&regex);
	free(matches);
	err:
	return true;
}

static struct ast_create_node* __must_check create_table_ast(struct stack *parser, int *i, struct stack *tmp_st)
{

	struct ast_create_node *node;
	char *content;
	struct stack reg_pars = {0};

	/* sanity checks */
	BUG_ON(stack_empty(parser));

	stack_init(&reg_pars);
	content = (char*)stack_peek_pos(parser, *i);

	if (!(node = zalloc(sizeof(*node))))
		goto err;

	node->node_type = AST_TYPE_CREATE;

	regex_helper(content, "CREATE ([0-9]+) ([0-9]+) ([A-Za-z][A-Za-z0-9_]*)", &reg_pars);

	node->if_not_exists = atoi((char*)stack_peek_pos(tmp_st, 0)) == 1;
	node->column_count = atoi((char*)stack_peek_pos(tmp_st, 1));
	strncpy(node->table_name,
		(char*)stack_peek_pos(tmp_st, 2),
		strlen((char*)stack_peek_pos(tmp_st, 2)) + 1);

	//TODO for tomorrow Add columns to node's children

	/* cleanup */
	stack_free(&reg_pars);

	return node;

	err:
	stack_free(&reg_pars);
	return NULL;

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
			stack_init(&tmp_st);
			regex_helper(str, "COLUMNDEF ([0-9]+) ([A-Za-z][A-Za-z0-9_]*)", &tmp_st);
			parse_bison_data_type((char*)stack_peek_pos(&tmp_st, 0), node);
			strncpy(node->name,
				(char*)stack_peek_pos(&tmp_st, 1),
				strlen((char*)stack_peek_pos(&tmp_st, 1)) + 1);
			stack_free(&tmp_st);
			break;
		}
	}

	return node;

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
	stack_init(&st);

	for (int i = 0; i < parser->idx; i++) {
		str = (char*)stack_peek_pos(parser, i);

		if (strstarts(str, "STARTCOL")) {
			struct ast_column_def_node *node = create_column_ast(parser, &i);
			if (!node)
				goto err;

			if (!stack_unsafe_push(&st, node))
				goto err;

		} else if (strstarts(str, "CREATE")) {

		} else {
			fprintf(stderr, "ast_build_tree: %s handler not implement yet\n", (char*)stack_peek(parser));
			exit(1);
		}
	}

	stack_free(&st);

	return root;

	err:
	stack_free(&st);
	//TODO implement free ast routine recursively
	free(root);

	return NULL;
}
