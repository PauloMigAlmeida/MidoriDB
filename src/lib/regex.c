/*
 * regex.c
 *
 *  Created on: 2/06/2023
 *      Author: paulo
 */

#include <lib/regex.h>

bool __must_check regex_ext_match_grp(const char *text, const char *exp, struct stack *out)
{

	regex_t regex;
	regmatch_t *matches;
	char *match;
	int num_matches;

	BUG_ON(regcomp(&regex, exp, REG_EXTENDED));
	num_matches = regex.re_nsub + 1;

	matches = calloc(num_matches, sizeof(*matches));
	if (!matches)
		goto err;

	if (regexec(&regex, text, num_matches, matches, 0))
		goto err_regex;

	for (int i = 1; i < num_matches; i++) {
		int start = matches[i].rm_so;
		int end = matches[i].rm_eo;
		int length = end - start;

		match = zalloc(length + 1);
		if (!match)
			goto err_regex;

		strncpy(match, text + start, length);

		if (!stack_push(out, match, length + 1))
			goto err_stack;

		printf("Match %d: %s\n", i, match);
		free(match);
	}

	/* post sanity check */
	BUG_ON((size_t )out->idx != regex.re_nsub - 1);

	regfree(&regex);
	free(matches);

	return true;

	err_stack:
	free(match);
	err_regex:
	stack_free(out);
	regfree(&regex);
	free(matches);
	err:
	return false;

}
