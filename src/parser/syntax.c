/*
 * syntax.c
 *
 *  Created on: 26/04/2023
 *      Author: paulo
 */

#include <parser/syntax.h>
#include <midorisql.tab.h>
#include <midorisql.yy.h>
#include <errno.h>

int syntax_parse(char *in, struct queue *out)
{
	yyscan_t sc;
	YY_BUFFER_STATE bs;
	int res;

	/* sanity check */
	BUG_ON(!in || !out);

	if (yylex_init(&sc)) {
		res = errno;
		if (!queue_offer(out, strerror(res), strlen(strerror(res))))
			fprintf(stderr, "error while gathering parser error \n");
		return res;
	}

	bs = yy_scan_string(in, sc);
	res = yyparse(out, sc);
	yy_delete_buffer(bs, sc);
	yylex_destroy(sc);

	return res;
}
