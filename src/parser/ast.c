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

static bool __must_check create_table_ast(struct stack *out, struct ast_node **root)
{
	regex_t regex;
	regmatch_t matches[4];

	/* sanity checks */
	BUG_ON(!root || stack_empty(out));

	char *content = (char*)stack_pop(out);

	// Compile the regular expression pattern
	if (regcomp(&regex, "CREATE\\s(\\d+)", REG_EXTENDED) != 0) {
		printf("Could not compile regex.\n");
		return true;
	}

	// Execute the regular expression and obtain matches
	int status = regexec(&regex, content, ARR_SIZE(matches), matches, 0);
	while (status == 0) {
		// Extract the captured matches
		for (int i = 1; i < 4; i++) {
			int start = matches[i].rm_so;
			int end = matches[i].rm_eo;
			int length = end - start;
			char *match = malloc(length + 1);
			strncpy(match, content + start, length);
			match[length] = '\0';

			printf("Match %d: %s\n", i, match);
			free(match);
		}
	}

	// Cleanup
	free(content);
	regfree(&regex);

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
