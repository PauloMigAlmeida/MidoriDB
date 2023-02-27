/*
 * heap.h
 *
 *  Created on: 27/02/2023
 *      Author: paulo
 */

#ifndef INCLUDE_COMPILER_HEAP_H_
#define INCLUDE_COMPILER_HEAP_H_

#include <string.h>

#define memzero(dest, size)	memset(dest, '\0', size)

#define zalloc(size) __extension__({			\
        void *__mptr;					\
        __mptr = malloc(size);				\
        if (__mptr)					\
		memzero(__mptr, size);			\
	__mptr;						\
})

#endif /* INCLUDE_COMPILER_HEAP_H_ */
