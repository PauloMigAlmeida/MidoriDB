#ifndef COMPILER_COMMON_H
#define COMPILER_COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include "compiler/bug.h"
#include "compiler/container_of.h"
#include "compiler/error.h"
#include "compiler/heap.h"
#include "compiler/compiler_attributes.h"

/* calculate the length of the array - and avoid tendinitis ;) */
#define ARR_SIZE(arr)		sizeof(arr)/sizeof(arr[0])

#endif /* COMPILER_COMMON_H */
