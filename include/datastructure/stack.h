/*
 * stack.h
 *
 *  Created on: 1/05/2023
 *      Author: paulo
 */

#ifndef INCLUDE_DATASTRUCTURE_STACK_H_
#define INCLUDE_DATASTRUCTURE_STACK_H_

#include <compiler/common.h>
#include <datastructure/vector.h>

/*
 * Dynamic-sized stack data structure
 */
struct stack {
	struct vector *arr;
	int idx;
};

/**
 * stack_init - initialise stack
 * @stack: stack to be initialised
 *
 * this function returns true if stack could be initialised, false otherwise
 */
bool __must_check stack_init(struct stack *stack);

/**
 * stack_free - free stack
 * @stack: stack to be freed
 */
void stack_free(struct stack *stack);

/**
 * stack_empty - check if stack is empty
 * @stack: stack reference
 */
bool stack_empty(struct stack *stack);

/**
 * stack_push - push content into stack
 * @stack: stack reference
 * @data: data to be pushed
 * @len: amount of data to be copied from data ptr
 *
 * this function returns true if data could be inserted into stack, false otherwise
 */
bool __must_check stack_push(struct stack *stack, void *data, size_t len);

/**
 * stack_unsafe_push - push content into stack without making a copy
 * @stack: stack reference
 * @data: data to be pushed
 *
 * this function returns true if data could be inserted into stack, false otherwise
 *
 * note: this function is meant to be used in very specific occasions. Use it with caution
 */
bool __must_check stack_unsafe_push(struct stack *stack, void *data);

/**
 * stack_pop - pop content from stack
 * @stack: stack reference
 *
 * this function returns a pointer or NULL if stack is empty. Also, as soon as the
 * pointer is out of the stack, it's the callers responsibility to free it
 */
uintptr_t* __must_check stack_pop(struct stack *stack);

/**
 * stack_peek - peek at top of stack without removing it
 * @stack: stack reference
 */
uintptr_t* stack_peek(struct stack *stack);

/**
 * stack_peek_pos - peek at index of stack without removing it
 * @stack: stack reference
 * @pos: index of the stack
 */
uintptr_t* stack_peek_pos(struct stack *stack, int pos);

#endif /* INCLUDE_DATASTRUCTURE_STACK_H_ */
