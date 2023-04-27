/*
 * syntax.c
 *
 *  Created on: 26/04/2023
 *      Author: paulo
 */

#include <parser/syntax.h>

int syntax_parse(char *stmt)
{
	//TODO implement this correctly later
	int ret;
	struct vector vec = {0};
	if(!vector_init(&vec))
		return 2;

	ret = bison_parse_string(stmt, &vec);

	printf("syntax_parse output: %s\n", vec.data);
	vector_free(&vec);
	return ret;
}
