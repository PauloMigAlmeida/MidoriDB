/*
 * syntax.c
 *
 *  Created on: 26/04/2023
 *      Author: paulo
 */

#include <parser/syntax.h>
#include <datastructure/stack.h>
#include <midorisql.tab.h>
#include <midorisql.yy.h>
#include <errno.h>

int bison_parse_string(const char *in, struct stack *out)
{
	yyscan_t sc;
	YY_BUFFER_STATE bs;
	int res;

	if (yylex_init(&sc)) {
		res = errno;
		stack_push(out, strerror(res), strlen(strerror(res)));
		return res;
	}

	/*
	 * TODO yy_scan_string calls malloc internally however, if it fails, it
	 * exits straight away instead of returning NULL or something.
	 * Check if we can override this behaviour later
	 */
	bs = yy_scan_string(in, sc);
	res = yyparse(out, sc);
	yy_delete_buffer(bs, sc);
	yylex_destroy(sc);

	return res;
}

static void print_stack_content(struct stack *st)
{
	// meant to be used while I'm developing the AST parser only
	// after that, let's grab a wrecking ball and demolish this
	// abomination

	for (int i = st->idx; i >= 0; i--) {
		char *ptr = *((char**)(st->arr->data + i * sizeof(uintptr_t)));
		printf("%s\n", ptr);
	}
	printf("\n");
}

int syntax_parse(char *stmt)
{
	//TODO implement this correctly later
	int ret;
	struct stack st = {0};

	if (!stack_init(&st))
		return 2;

	ret = bison_parse_string(stmt, &st);

	printf("%s\n", stmt);
	print_stack_content(&st);
	stack_free(&st);
	return ret;
}
