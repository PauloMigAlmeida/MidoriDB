/*
 * syntax.c
 *
 *  Created on: 26/04/2023
 *      Author: paulo
 */

#include <parser/syntax.h>
#include <midorisql.tab.h>

int syntax_parse(char *stmt)
{
	return parse_string(stmt);
}
