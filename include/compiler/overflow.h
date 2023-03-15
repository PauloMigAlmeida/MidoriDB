/*
 * overflow.h
 *
 *  Created on: 15/03/2023
 *      Author: paulo
 */

#ifndef INCLUDE_COMPILER_OVERFLOW_H_
#define INCLUDE_COMPILER_OVERFLOW_H_

/*
 * calculate size of flex-array structures
 *
 * @p: struct pointer
 * @member: flex array member of pointer p
 * @count: how many times repeats
 */
#define struct_size(p, member, count) __extension__({		\
	__must_be_array((p)->member); 				\
	sizeof(*(p)) + sizeof(*(p)->member) * count;		\
})

/*
 * equivalent to struct_size but you can use that without
 * having a variable of type struct __ * which helps during
 * compilation
 */
#define struct_size_const(s, member, count)	struct_size((s*)0, member, count)

#endif /* INCLUDE_COMPILER_OVERFLOW_H_ */
