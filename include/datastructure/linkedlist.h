#ifndef DATASTRUCTURE_LINKEDLIST_H
#define DATASTRUCTURE_LINKEDLIST_H

#include "compiler/common.h"

/*
 * Circular doubly linked list implementation
 * mostly taken for the linux kernel source code
 * but minimised/simplified in some ways.
 */
struct list_head {
	struct list_head *next;
	struct list_head *prev;
};

void list_head_init(struct list_head *head);
void list_add(struct list_head *new, struct list_head *head);
void list_del(struct list_head *entry);
size_t list_length(struct list_head *head);

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

#define list_for_each(pos, head) \
        for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:	the &struct list_head to use as a loop cursor.
 * @n:		another &struct list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_safe(pos, n, head) 	\
	for (pos = (head)->next, n = pos->next; \
	     pos != (head); 			\
	     pos = n, n = pos->next)

#define list_entry(ptr, type, member) 		\
        container_of(ptr, type, member)

/**
 * list_is_empty - check if circular list is empty
 * @head:	the head of your list
 *
 * this function returns true if list is empty, false otherwise
 */
#define list_is_empty(head)			\
	((head) == (head)->next)

#endif /* DATASTRUCTURE_LINKEDLIST_H */ 
