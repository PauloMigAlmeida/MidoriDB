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

static bool regex_helper(const char *text, const char *exp, struct stack *out)
{
	regex_t regex;
	regmatch_t *matches;
	char *match;

	BUG_ON(regcomp(&regex, exp, REG_EXTENDED));

	matches = calloc(regex.re_nsub + 1, sizeof(*matches));
	if (!matches)
		goto err;

	if (regexec(&regex, text, regex.re_nsub + 1, matches, 0))
		goto err_regex;

	for (size_t i = 1; i < regex.re_nsub + 1; i++) {
		int start = matches[i].rm_so;
		int end = matches[i].rm_eo;
		int length = end - start;

		match = zalloc(length + 1);
		if (!match)
			goto err_regex;

		strncpy(match, text + start, length);

		if (!stack_push(out, match, length + 1))
			goto err_stack;

		printf("Match %lu: %s\n", i, match);
		free(match);
	}

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


static bool __must_check create_table_ast(struct stack *out, struct ast_node **root)
{

	struct ast_create_node *node;
	char *content;
	struct stack st = {0};

	/* sanity checks */
	BUG_ON(!root || stack_empty(out));

	stack_init(&st);
	content = (char*)stack_pop(out);
	
	if(!(node = zalloc(sizeof(*node))))
		return true;
	
	node->node_type = AST_TYPE_CREATE;

	regex_helper(content, "CREATE ([0-9]+) ([0-9]+) ([A-Za-z]+)", &st);

	/* cleanup */
	free(content);
	stack_free(&st);

	return false;
}

struct ast_node* ast_build_tree(struct stack *out)
{
	struct ast_node *root = NULL;

	/* sanity checks */
	BUG_ON(!out || stack_empty(out));
	BUG_ON(!strstarts((char* )stack_peek(out), "STMT"));

	/* free STMT */
	free(stack_pop(out));

	if (strstarts((char*)stack_peek(out), "CREATE")) {
		if (create_table_ast(out, &root))
			goto err;
	} else {
		fprintf(stderr, "ast_build_tree: %s handler not implement yet\n", (char*)stack_peek(out));
		exit(1);
	}

	return root;

	err:
	//TODO implement free ast routine recursively
	free(root);

	return NULL;
}
