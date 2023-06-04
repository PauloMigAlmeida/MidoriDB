#include <tests/lib.h>
#include <lib/string.h>

void test_strrand(void) {
    char str[10];
    strrand(str, 10);
    CU_ASSERT(strlen(str) == 10);
}

void test_strstarts(void) {
    CU_ASSERT(strstarts("hello world", "hello"));
    CU_ASSERT(!strstarts("hello world", "world"));
}
