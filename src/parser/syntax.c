/*
 * syntax.c
 *
 *  Created on: 26/04/2023
 *      Author: paulo
 */

#include <parser/syntax.h>
#include <midorisql.tab.h>
#include <midorisql.yy.h>

int bison_parse_string(const char *in, struct vector *out)
{
	//TODO take care of failures during initialisation
	yyscan_t sc;
	YY_BUFFER_STATE bs;
	int res;

	yylex_init(&sc);
	bs = yy_scan_string(in, sc);
	res = yyparse(out, sc);
	yy_delete_buffer(bs, sc);
	yylex_destroy(sc);

	return res;
}

int syntax_parse(char *stmt)
{
	//TODO implement this correctly later
	int ret;
	struct vector vec = {0};

	if (!vector_init(&vec))
		return 2;

	ret = bison_parse_string(stmt, &vec);

	printf("%s\n", stmt);
	printf("%s\n\n", vec.data);
	vector_free(&vec);
	return ret;
}
