/*
 * queue.h
 *
 *  Created on: 31/05/2023
 *      Author: paulo
 */

#ifndef INCLUDE_DATASTRUCTURE_QUEUE_H_
#define INCLUDE_DATASTRUCTURE_QUEUE_H_

#include <compiler/common.h>
#include <datastructure/vector.h>

/*
 * Dynamic-sized queue data structure
 */
struct queue {
	struct vector *arr;
};

/**
 * queue_init - initialise queue
 * @queue: queue to be initialised
 *
 * this function returns true if queue could be initialised, false otherwise
 */
bool __must_check queue_init(struct queue *queue);

/**
 * queue_free - free queue
 * @queue: queue to be freed
 */
void queue_free(struct queue *queue);


/**
 * queue_empty - check if queue is empty
 * @queue: queue reference
 */
bool queue_empty(struct queue *queue);

/**
 * queue_offer - add content into queue
 * @queue: queue reference
 * @data: data to be pushed
 * @len: amount of data to be copied from data ptr
 *
 * this function returns true if data could be inserted into queue, false otherwise
 */
bool __must_check queue_offer(struct queue *queue, void *data, size_t len);

/**
 * queue_poll - remove content from queue
 * @queue: queue reference
 * 
 * this function returns the element at the head of the queue, or NULL if queue is empty
 */
void* __must_check queue_poll(struct queue *queue);

/**
 * queue_peek - get content from head of queue
 * @queue: queue reference
 * 
 * this function returns the element at the head of the queue, or NULL if queue is empty
 */
void* __must_check queue_peek(struct queue *queue);

/**
 * queue_peek_pos - get content from queue at a given position
 * @queue: queue reference
 * @pos: position to peek
 * 
 * this function returns the element at the given position of the queue, or NULL if position is invalid
 */
void* __must_check queue_peek_pos(struct queue *queue, size_t pos);


#endif /* INCLUDE_DATASTRUCTURE_QUEUE_H_ */
