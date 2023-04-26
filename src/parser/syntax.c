/*
 * syntax.c
 *
 *  Created on: 26/04/2023
 *      Author: paulo
 */

#include <parser/syntax.h>

int syntax_parse(char *stmt)
{
	return bison_parse_string(stmt);
}
