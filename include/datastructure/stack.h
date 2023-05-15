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
 * Dynamic-size stack data structure
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
bool stack_init(struct stack *stack);

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
bool stack_push(struct stack *stack, void *data, size_t len);

/**
 * stack_pop - pop content from stack
 * @stack: stack reference
 *
 * this function returns a pointer or NULL if stack is empty. Also, as soon as the
 * pointer is out of the stack, it's the callers responsability to free it
 */
uintptr_t* stack_pop(struct stack *stack);

/**
 * stack_peel - peek at content from stack without removing it
 * @stack: stack reference
 */
uintptr_t* stack_peek(struct stack *stack);

#endif /* INCLUDE_DATASTRUCTURE_STACK_H_ */
