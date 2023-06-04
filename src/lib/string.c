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

void strrand(char *str, size_t len)
{
	static const char charset[] = "abcdefghijklmnopqrstuvwxyz"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"0123456789";
	size_t charset_len = sizeof(charset) - 1;

	for (size_t i = 0; i < len; i++)
		str[i] = charset[rand() % charset_len];

	str[len] = '\0';
}
