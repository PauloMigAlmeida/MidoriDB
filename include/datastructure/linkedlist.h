#ifndef MIDORIDB_DATASTRUCTURE_LINKEDLIST_H
#define MIDORIDB_DATASTRUCTURE_LINKEDLIST_H

#include <stddef.h>

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

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

#define list_for_each(pos, head) \
        for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_entry(ptr, type, member) \
        container_of(ptr, type, member)

#endif /* MIDORIDB_DATASTRUCTURE_LINKEDLIST_H */ 
