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
#define __must_check	__attribute__((__warn_unused_result__))

#endif /* INCLUDE_COMPILER_COMPILER_ATTRIBUTES_H_ */
