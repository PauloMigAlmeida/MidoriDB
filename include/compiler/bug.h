#ifndef COMPILER_BUG_H
#define COMPILER_BUG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include "compiler/macro.h"

static inline void die(const char *fmt, ...)
{
	va_list ap;
	int ret = errno;

	if (errno)
		perror("midoridb");
	else
		ret = -1;

	va_start(ap, fmt);
	fprintf(stderr, "  ");
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, "\n");
	exit(ret);
}

#define BUG_ON(cond)								\
	do {									\
		if (unlikely(cond))						\
			die("BUG_ON: %s:%u:%s", __FILE__, __LINE__, __func__);	\
	} while (0)

#define BUG_ON_CUSTOM_MSG(cond, msg)								\
	do {											\
		if (unlikely(cond))								\
			die("BUG_ON: %s:%u:%s :: %s", __FILE__, __LINE__, __func__, msg);	\
	} while (0)

#endif /* COMPILER_BUG_H */
