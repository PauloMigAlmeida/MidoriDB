#ifndef COMPILER_MACRO_H
#define COMPILER_MACRO_H

/* calculate the length of the array - and avoid tendinitis ;) */
#define ARR_SIZE(arr)		    sizeof(arr)/sizeof(arr[0])

/* check if value is a power of 2 */
#define IS_POWER_OF_2(x)        (x != 0) && ((x & (x - 1)) == 0)

/* throws an error at compilation phase if it evalutates to false */
#define BUILD_BUG(expr, msg)    _Static_assert(expr, msg);

#endif /* COMPILER_MACRO_H */
