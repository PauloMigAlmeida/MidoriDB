#include "lib/bit.h"

bool bit_test(void *field, size_t bit_number, size_t len)
{
	size_t byte_no = floorl(bit_number / CHAR_BIT);
	bit_number -= byte_no * CHAR_BIT;
	BUG_ON(len <= byte_no);
	return (((char*)field)[byte_no] >> bit_number) & 1ULL;
}

void bit_set(void *field, uint8_t bit_number, size_t len)
{
	size_t byte_no = floorl(bit_number / CHAR_BIT);
	bit_number -= byte_no * CHAR_BIT;
	BUG_ON(len <= byte_no);
	((char*)field)[byte_no] |= (1ULL << bit_number);
}

void bit_clear(void *field, uint8_t bit_number, size_t len)
{
	size_t byte_no = floorl(bit_number / CHAR_BIT);
	bit_number -= byte_no * CHAR_BIT;
	BUG_ON(len <= byte_no);
	((char*)field)[byte_no] &= ~(1UL << bit_number);
}

void bit_toggle(void *field, uint8_t bit_number, size_t len)
{
	size_t byte_no = floorl(bit_number / CHAR_BIT);
	bit_number -= byte_no * CHAR_BIT;
	BUG_ON(len <= byte_no);
	((char*)field)[byte_no] ^= (1UL << bit_number);
}
