#ifndef MIDORIDB_DATASTRUCTURE_LINKEDLIST_H
#define MIDORIDB_DATASTRUCTURE_LINKEDLIST_H

#include <stddef.h>

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

#endif /* MIDORIDB_DATASTRUCTURE_LINKEDLIST_H */ 
