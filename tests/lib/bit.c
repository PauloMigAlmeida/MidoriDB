#include "tests/lib.h"
#include "lib/bit.h"

void test_bit_test(void)
{    
    /* values that fit existing C types */
    uint64_t value_s = 0xaa; // 0b10101010
    CU_ASSERT(bit_test(&value_s, 7, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 6, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 5, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 4, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 3, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 2, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 1, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 0, sizeof(value_s)));

    value_s = 0x55; // 0b01010101
    CU_ASSERT(!bit_test(&value_s, 7, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 6, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 5, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 4, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 3, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 2, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 1, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 0, sizeof(value_s)));

    /* values that don't fit existing C types */
    char value_c[] = {0xaa, 0xaa};
    CU_ASSERT(bit_test(&value_c, 15, sizeof(value_c)));
    CU_ASSERT(!bit_test(&value_c, 14, sizeof(value_c)));
    CU_ASSERT(bit_test(&value_c, 13, sizeof(value_c)));
    CU_ASSERT(!bit_test(&value_c, 12, sizeof(value_c)));
    CU_ASSERT(bit_test(&value_c, 11, sizeof(value_c)));
    CU_ASSERT(!bit_test(&value_c, 10, sizeof(value_c)));
    CU_ASSERT(bit_test(&value_c, 9, sizeof(value_c)));
    CU_ASSERT(!bit_test(&value_c, 8, sizeof(value_c)));
    CU_ASSERT(bit_test(&value_c, 7, sizeof(value_c)));
    CU_ASSERT(!bit_test(&value_c, 6, sizeof(value_c)));
    CU_ASSERT(bit_test(&value_c, 5, sizeof(value_c)));
    CU_ASSERT(!bit_test(&value_c, 4, sizeof(value_c)));
    CU_ASSERT(bit_test(&value_c, 3, sizeof(value_c)));
    CU_ASSERT(!bit_test(&value_c, 2, sizeof(value_c)));
    CU_ASSERT(bit_test(&value_c, 1, sizeof(value_c)));
    CU_ASSERT(!bit_test(&value_c, 0, sizeof(value_c)));

    /* 
     * that reminds me of the MBR signature.
     *
     * one might think this represents (0b10101010 01010101)
     * but if you consider little-endian than it's in fact
     * (0b01010101 10101010).
     */
    char value_d[] = {0xaa, 0x55}; 
    CU_ASSERT(!bit_test(&value_d, 15, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 14, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 13, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 12, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 11, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 10, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 9, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 8, sizeof(value_d)));

    CU_ASSERT(bit_test(&value_d, 7, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 6, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 5, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 4, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 3, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 2, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 1, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 0, sizeof(value_d)));

    /*
     * originally (0b10101010 01010101 11111111), but
     * once little-endian is applied it becomes
     * (0b11111111 01010101 10101010)
     */
    char value_e[] = {0xaa, 0x55, 0xff}; 
    CU_ASSERT(bit_test(&value_e, 23, sizeof(value_e)));
    CU_ASSERT(bit_test(&value_e, 22, sizeof(value_e)));
    CU_ASSERT(bit_test(&value_e, 21, sizeof(value_e)));
    CU_ASSERT(bit_test(&value_e, 20, sizeof(value_e)));
    CU_ASSERT(bit_test(&value_e, 19, sizeof(value_e)));
    CU_ASSERT(bit_test(&value_e, 18, sizeof(value_e)));
    CU_ASSERT(bit_test(&value_e, 17, sizeof(value_e)));
    CU_ASSERT(bit_test(&value_e, 16, sizeof(value_e)));

    CU_ASSERT(!bit_test(&value_e, 15, sizeof(value_e)));
    CU_ASSERT(bit_test(&value_e, 14, sizeof(value_e)));
    CU_ASSERT(!bit_test(&value_e, 13, sizeof(value_e)));
    CU_ASSERT(bit_test(&value_e, 12, sizeof(value_e)));
    CU_ASSERT(!bit_test(&value_e, 11, sizeof(value_e)));
    CU_ASSERT(bit_test(&value_e, 10, sizeof(value_e)));
    CU_ASSERT(!bit_test(&value_e, 9, sizeof(value_e)));
    CU_ASSERT(bit_test(&value_e, 8, sizeof(value_e)));

    CU_ASSERT(bit_test(&value_e, 7, sizeof(value_e)));
    CU_ASSERT(!bit_test(&value_e, 6, sizeof(value_e)));
    CU_ASSERT(bit_test(&value_e, 5, sizeof(value_e)));
    CU_ASSERT(!bit_test(&value_e, 4, sizeof(value_e)));
    CU_ASSERT(bit_test(&value_e, 3, sizeof(value_e)));
    CU_ASSERT(!bit_test(&value_e, 2, sizeof(value_e)));
    CU_ASSERT(bit_test(&value_e, 1, sizeof(value_e)));
    CU_ASSERT(!bit_test(&value_e, 0, sizeof(value_e)))
}

void test_bit_set(void)
{
    /* values that fit existing C types */
    uint64_t value_s = 0xaa; // 0b10101010
    CU_ASSERT(bit_test(&value_s, 7, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 6, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 5, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 4, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 3, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 2, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 1, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 0, sizeof(value_s)));

    bit_set(&value_s, 6, sizeof(value_s));
    CU_ASSERT(bit_test(&value_s, 7, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 6, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 5, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 4, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 3, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 2, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 1, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 0, sizeof(value_s)));

    bit_set(&value_s, 4, sizeof(value_s));
    CU_ASSERT(bit_test(&value_s, 7, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 6, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 5, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 4, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 3, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 2, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 1, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 0, sizeof(value_s)));

    bit_set(&value_s, 2, sizeof(value_s));
    CU_ASSERT(bit_test(&value_s, 7, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 6, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 5, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 4, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 3, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 2, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 1, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 0, sizeof(value_s)));
    
    bit_set(&value_s, 0, sizeof(value_s));
    CU_ASSERT(bit_test(&value_s, 7, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 6, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 5, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 4, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 3, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 2, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 1, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 0, sizeof(value_s)));

    /* values that don't fit existing C types */
    char value_d[] = {0xaa, 0x55}; 
    CU_ASSERT(!bit_test(&value_d, 15, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 14, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 13, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 12, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 11, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 10, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 9, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 8, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 7, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 6, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 5, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 4, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 3, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 2, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 1, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 0, sizeof(value_d)));

    bit_set(&value_d, 15, sizeof(value_d));
    CU_ASSERT(bit_test(&value_d, 15, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 14, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 13, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 12, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 11, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 10, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 9, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 8, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 7, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 6, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 5, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 4, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 3, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 2, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 1, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 0, sizeof(value_d)));

    bit_set(&value_d, 13, sizeof(value_d));
    CU_ASSERT(bit_test(&value_d, 15, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 14, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 13, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 12, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 11, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 10, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 9, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 8, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 7, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 6, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 5, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 4, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 3, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 2, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 1, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 0, sizeof(value_d)));

    bit_set(&value_d, 11, sizeof(value_d));
    CU_ASSERT(bit_test(&value_d, 15, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 14, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 13, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 12, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 11, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 10, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 9, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 8, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 7, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 6, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 5, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 4, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 3, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 2, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 1, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 0, sizeof(value_d)));

    bit_set(&value_d, 9, sizeof(value_d));
    CU_ASSERT(bit_test(&value_d, 15, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 14, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 13, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 12, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 11, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 10, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 9, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 8, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 7, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 6, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 5, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 4, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 3, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 2, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 1, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 0, sizeof(value_d)));

    bit_set(&value_d, 6, sizeof(value_d));
    CU_ASSERT(bit_test(&value_d, 15, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 14, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 13, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 12, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 11, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 10, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 9, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 8, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 7, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 6, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 5, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 4, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 3, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 2, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 1, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 0, sizeof(value_d)));
    
    bit_set(&value_d, 4, sizeof(value_d));
    CU_ASSERT(bit_test(&value_d, 15, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 14, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 13, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 12, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 11, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 10, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 9, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 8, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 7, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 6, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 5, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 4, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 3, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 2, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 1, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 0, sizeof(value_d)));

    bit_set(&value_d, 4, sizeof(value_d));
    CU_ASSERT(bit_test(&value_d, 15, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 14, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 13, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 12, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 11, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 10, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 9, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 8, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 7, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 6, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 5, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 4, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 3, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 2, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 1, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 0, sizeof(value_d)));

    bit_set(&value_d, 2, sizeof(value_d));
    CU_ASSERT(bit_test(&value_d, 15, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 14, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 13, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 12, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 11, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 10, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 9, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 8, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 7, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 6, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 5, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 4, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 3, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 2, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 1, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 0, sizeof(value_d)));

    bit_set(&value_d, 0, sizeof(value_d));
    CU_ASSERT(bit_test(&value_d, 15, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 14, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 13, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 12, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 11, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 10, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 9, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 8, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 7, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 6, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 5, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 4, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 3, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 2, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 1, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 0, sizeof(value_d)));
}

void test_bit_clear(void)
{
    /* values that fit existing C types */
    uint64_t value_s = 0xaa; // 0b10101010
    CU_ASSERT(bit_test(&value_s, 7, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 6, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 5, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 4, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 3, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 2, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 1, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 0, sizeof(value_s)));

    bit_clear(&value_s, 7, sizeof(value_s));
    CU_ASSERT(!bit_test(&value_s, 7, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 6, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 5, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 4, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 3, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 2, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 1, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 0, sizeof(value_s)));

    bit_clear(&value_s, 5, sizeof(value_s));
    CU_ASSERT(!bit_test(&value_s, 7, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 6, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 5, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 4, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 3, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 2, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 1, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 0, sizeof(value_s)));

    bit_clear(&value_s, 3, sizeof(value_s));
    CU_ASSERT(!bit_test(&value_s, 7, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 6, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 5, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 4, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 3, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 2, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 1, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 0, sizeof(value_s)));

    bit_clear(&value_s, 1, sizeof(value_s));
    CU_ASSERT(!bit_test(&value_s, 7, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 6, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 5, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 4, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 3, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 2, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 1, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 0, sizeof(value_s)));

    /* values that don't fit existing C types */
    char value_d[] = {0xaa, 0x55}; 
    CU_ASSERT(!bit_test(&value_d, 15, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 14, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 13, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 12, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 11, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 10, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 9, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 8, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 7, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 6, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 5, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 4, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 3, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 2, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 1, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 0, sizeof(value_d)));

    bit_clear(&value_d, 14, sizeof(value_d));
    CU_ASSERT(!bit_test(&value_d, 15, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 14, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 13, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 12, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 11, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 10, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 9, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 8, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 7, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 6, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 5, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 4, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 3, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 2, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 1, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 0, sizeof(value_d)));

    bit_clear(&value_d, 12, sizeof(value_d));
    CU_ASSERT(!bit_test(&value_d, 15, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 14, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 13, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 12, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 11, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 10, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 9, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 8, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 7, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 6, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 5, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 4, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 3, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 2, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 1, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 0, sizeof(value_d)));

    bit_clear(&value_d, 10, sizeof(value_d));
    CU_ASSERT(!bit_test(&value_d, 15, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 14, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 13, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 12, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 11, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 10, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 9, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 8, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 7, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 6, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 5, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 4, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 3, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 2, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 1, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 0, sizeof(value_d)));

    bit_clear(&value_d, 8, sizeof(value_d));
    CU_ASSERT(!bit_test(&value_d, 15, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 14, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 13, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 12, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 11, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 10, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 9, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 8, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 7, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 6, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 5, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 4, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 3, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 2, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 1, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 0, sizeof(value_d)));

    bit_clear(&value_d, 7, sizeof(value_d));
    CU_ASSERT(!bit_test(&value_d, 15, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 14, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 13, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 12, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 11, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 10, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 9, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 8, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 7, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 6, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 5, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 4, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 3, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 2, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 1, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 0, sizeof(value_d)));

    bit_clear(&value_d, 5, sizeof(value_d));
    CU_ASSERT(!bit_test(&value_d, 15, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 14, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 13, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 12, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 11, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 10, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 9, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 8, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 7, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 6, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 5, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 4, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 3, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 2, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 1, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 0, sizeof(value_d)));

    bit_clear(&value_d, 3, sizeof(value_d));
    CU_ASSERT(!bit_test(&value_d, 15, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 14, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 13, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 12, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 11, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 10, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 9, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 8, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 7, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 6, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 5, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 4, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 3, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 2, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 1, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 0, sizeof(value_d)));

    bit_clear(&value_d, 1, sizeof(value_d));
    CU_ASSERT(!bit_test(&value_d, 15, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 14, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 13, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 12, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 11, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 10, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 9, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 8, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 7, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 6, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 5, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 4, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 3, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 2, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 1, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 0, sizeof(value_d)));
}

void test_bit_toggle(void) {
    /* values that fit existing C types */
    uint64_t value_s = 0xaa; // 0b10101010
    CU_ASSERT(bit_test(&value_s, 7, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 6, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 5, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 4, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 3, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 2, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 1, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 0, sizeof(value_s)));

    bit_toggle(&value_s, 7, sizeof(value_s));
    bit_toggle(&value_s, 6, sizeof(value_s));
    bit_toggle(&value_s, 5, sizeof(value_s));
    bit_toggle(&value_s, 4, sizeof(value_s));
    bit_toggle(&value_s, 3, sizeof(value_s));
    bit_toggle(&value_s, 2, sizeof(value_s));
    bit_toggle(&value_s, 1, sizeof(value_s));
    bit_toggle(&value_s, 0, sizeof(value_s));

    CU_ASSERT(!bit_test(&value_s, 7, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 6, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 5, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 4, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 3, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 2, sizeof(value_s)));
    CU_ASSERT(!bit_test(&value_s, 1, sizeof(value_s)));
    CU_ASSERT(bit_test(&value_s, 0, sizeof(value_s)));

    /* values that don't fit existing C types */
    char value_d[] = {0xaa, 0x55}; 
    CU_ASSERT(!bit_test(&value_d, 15, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 14, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 13, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 12, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 11, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 10, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 9, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 8, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 7, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 6, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 5, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 4, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 3, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 2, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 1, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 0, sizeof(value_d)));

    bit_toggle(&value_d, 15, sizeof(value_d));
    bit_toggle(&value_d, 14, sizeof(value_d));
    bit_toggle(&value_d, 13, sizeof(value_d));
    bit_toggle(&value_d, 12, sizeof(value_d));
    bit_toggle(&value_d, 11, sizeof(value_d));
    bit_toggle(&value_d, 10, sizeof(value_d));
    bit_toggle(&value_d, 9, sizeof(value_d));
    bit_toggle(&value_d, 8, sizeof(value_d));
    bit_toggle(&value_d, 7, sizeof(value_d));
    bit_toggle(&value_d, 6, sizeof(value_d));
    bit_toggle(&value_d, 5, sizeof(value_d));
    bit_toggle(&value_d, 4, sizeof(value_d));
    bit_toggle(&value_d, 3, sizeof(value_d));
    bit_toggle(&value_d, 2, sizeof(value_d));
    bit_toggle(&value_d, 1, sizeof(value_d));
    bit_toggle(&value_d, 0, sizeof(value_d));

    CU_ASSERT(bit_test(&value_d, 15, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 14, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 13, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 12, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 11, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 10, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 9, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 8, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 7, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 6, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 5, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 4, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 3, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 2, sizeof(value_d)));
    CU_ASSERT(!bit_test(&value_d, 1, sizeof(value_d)));
    CU_ASSERT(bit_test(&value_d, 0, sizeof(value_d)));
}
