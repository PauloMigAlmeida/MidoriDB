/*
 * compiler_attributes.h
 *
 *  Created on: 3/03/2023
 *      Author: paulo
 */

#ifndef INCLUDE_COMPILER_COMPILER_ATTRIBUTES_H_
#define INCLUDE_COMPILER_COMPILER_ATTRIBUTES_H_

/*
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-warn_005funused_005fresult-function-attribute
 * clang: https://clang.llvm.org/docs/AttributeReference.html#nodiscard-warn-unused-result
 */
#define __must_check		__attribute__((__warn_unused_result__))

/* Are two types/vars the same type (ignoring qualifiers)? */
#define __same_type(a, b) 	__builtin_types_compatible_p(typeof(a), typeof(b))

/* &a[0] degrades to a pointer: a different type from an array */
#define __must_be_array(a) 	BUILD_BUG(!__same_type((a), &(a)[0]), "must be an array")

/* align struct, member or variable */
#define __align(num)		__attribute__ ((aligned (num)))

/* align struct,member or variable for better performance on x86-64 platform */
#define __x86_64_align 		__align(8)

/*
 * tells gcc to inline functions even when not optimizing
 * more: https://gcc.gnu.org/onlinedocs/gcc/Inline.html
 */
#define __force_inline		inline __attribute__((always_inline))

/*
 * This attribute, attached to an enum, struct, or union type definition,
 * specified that the minimum required memory be used to represent the type.
 * more: https://gcc.gnu.org/onlinedocs/gcc-3.3/gcc/Type-Attributes.html
 */
#define __packed            __attribute__((__packed__))

#endif /* INCLUDE_COMPILER_COMPILER_ATTRIBUTES_H_ */
