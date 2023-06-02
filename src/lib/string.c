/*
 * string.c
 *
 *  Created on: 2/06/2023
 *      Author: paulo
 */

#include <lib/string.h>

bool strstarts(const char *str, const char *prefix)
{
	return strncmp(str, prefix, strlen(prefix)) == 0;
}
