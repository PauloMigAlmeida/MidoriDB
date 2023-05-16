/*
 * stack.c
 *
 *  Created on: 1/05/2023
 *      Author: paulo
 */

#include <datastructure/stack.h>

bool stack_init(struct stack *stack)
{
	/* sanity checks */
	BUG_ON(!stack);

	stack->arr = zalloc(sizeof(*stack->arr));
	if (!stack->arr)
		goto err;

	if (!vector_init(stack->arr))
		goto err_vector;

	stack->idx = -1;

	return true;

	err_vector:
	free(stack->arr);
	err:
	return false;
}

void stack_free(struct stack *stack)
{
	/* sanity checks */
	BUG_ON(!stack);

	while (!stack_empty(stack))
	{
		free(stack_pop(stack));
	}

	vector_free(stack->arr);
	free(stack->arr);
}

bool stack_empty(struct stack *stack)
{
	/* sanity checks */
	BUG_ON(!stack);

	return stack->idx == -1;
}

bool stack_push(struct stack *stack, void *data, size_t len)
{
	char *content;

	/* sanity checks */
	BUG_ON(!stack || !data || len == 0);

	content = zalloc(len);
	if (!content)
		goto err;

	memcpy(content, data, len);

	if (!vector_push(stack->arr, &content, sizeof(uintptr_t)))
		goto err_vector;

	stack->idx++;
	return true;

	err_vector:
	free(content);
	err:
	return false;
}

uintptr_t* stack_pop(struct stack *stack)
{
	uintptr_t *ptr;

	/* sanity checks */
	BUG_ON(!stack || stack_empty(stack));

	ptr = *((uintptr_t**)(stack->arr->data + stack->idx * sizeof(uintptr_t)));
	stack->idx--;
	stack->arr->len -= sizeof(uintptr_t);

	return ptr;
}

uintptr_t* stack_peek(struct stack *stack)
{
	/* sanity checks */
	BUG_ON(!stack || stack_empty(stack));

	return *((uintptr_t**)(stack->arr->data + stack->idx * sizeof(uintptr_t)));
}

uintptr_t* stack_peek_pos(struct stack *stack, int pos)
{
	/* sanity checks */
	BUG_ON(!stack || stack_empty(stack) || pos < 0 || pos > stack->idx);

	return *((uintptr_t**)(stack->arr->data + pos * sizeof(uintptr_t)));
}

