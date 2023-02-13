#ifndef COMPILER_BUG_H
#define COMPILER_BUG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

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


#endif /* COMPILER_BUG_H */
