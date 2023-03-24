#include "lib/bit.h"

bool bit_test(void* field, size_t bit_number, size_t len) {
    size_t byte_no = floorl(bit_number / CHAR_BIT);    
    bit_number -= byte_no * CHAR_BIT;
    BUG_ON(len < byte_no);
    // printf("byte_no: %zu, bit_number: %zu, len: %zu, final: %zu\n", byte_no, bit_number, len, (len - byte_no - 1));
    char* field_ptr = (char*)field;
    return (field_ptr[byte_no] >> bit_number) & 1ULL;
}

void bit_set(void* field, uint8_t bit_number) {
    size_t byte_no = (size_t)floorl(bit_number / CHAR_BIT);
    bit_number -= byte_no * CHAR_BIT;
    char* field_ptr = (char*)field;
    field_ptr[byte_no] |= (1ULL << bit_number);
}

void bit_clear(void* field, uint8_t bit_number) {
    size_t byte_no = (size_t)floorl(bit_number / CHAR_BIT);
    bit_number -= byte_no * CHAR_BIT;
    char* field_ptr = (char*)field;
    field_ptr[byte_no] &= ~(1UL << bit_number);
}

void bit_toggle(void* field, uint8_t bit_number) {
    size_t byte_no = (size_t)floorl(bit_number / CHAR_BIT);
    bit_number -= byte_no * CHAR_BIT;
    char* field_ptr = (char*)field;
    field_ptr[byte_no] ^= (1UL << bit_number);
}
