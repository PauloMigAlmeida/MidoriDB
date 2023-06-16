/*
 * queue.c
 *
 *  Created on: 31/05/2023
 *      Author: paulo
 */

#include <datastructure/queue.h>

bool __must_check queue_init(struct queue *queue)
{
	/* sanity checks */
	BUG_ON(!queue);

	queue->arr = zalloc(sizeof(*queue->arr));
	if (!queue->arr)
		goto err;

	if (!vector_init(queue->arr))
		goto err_vector;

	return true;

err_vector:
	free(queue->arr);
err:
	return false;
}

void queue_free(struct queue *queue)
{
	/* sanity checks */
	BUG_ON(!queue);

	while (!queue_empty(queue))
	{
		free(queue_poll(queue));
	}

	vector_free(queue->arr);
	free(queue->arr);
	queue->arr = NULL;
}

bool queue_empty(struct queue *queue)
{
	/* sanity check */
	BUG_ON(!queue);
	return queue->arr->len == 0;
}

bool __must_check queue_offer(struct queue *queue, void *data, size_t len)
{
	char *content;

	/* sanity checks */
	BUG_ON(!queue || !data || len == 0);

	content = zalloc(len);
	if (!content)
		goto err;

	memcpy(content, data, len);

	if (!vector_push(queue->arr, &content, sizeof(uintptr_t)))
		goto err_vector;

	return true;

err_vector:
	free(content);
err:
	return false;
}

void* __must_check queue_poll(struct queue *queue)
{
	uintptr_t *ptr;

	/* sanity checks */
	BUG_ON(!queue);

	if (queue_empty(queue))
		return NULL;

	ptr = *((uintptr_t**)queue->arr->data);
	memmove(queue->arr->data,
		queue->arr->data + sizeof(uintptr_t),
		queue->arr->len - sizeof(uintptr_t));
	queue->arr->len -= sizeof(uintptr_t);

	return ptr;
}

void* __must_check queue_peek(struct queue *queue)
{
	/* sanity checks */
	BUG_ON(!queue);

	if (queue_empty(queue))
		return NULL;

	return *((uintptr_t**)queue->arr->data);
}

void* __must_check queue_peek_pos(struct queue *queue, size_t pos)
{
	/* sanity checks */
	BUG_ON(!queue || queue_empty(queue) || pos > queue_length(queue) - 1);

	return *((uintptr_t**)(queue->arr->data + pos * sizeof(uintptr_t)));
}

size_t queue_length(struct queue *queue)
{
	/* sanity checks */
	BUG_ON(!queue);

	return queue->arr->len / sizeof(uintptr_t);
}
