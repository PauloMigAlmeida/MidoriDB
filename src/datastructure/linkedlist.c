#include "datastructure/linkedlist.h"

void list_head_init(struct list_head *head) {
	head->next = head;
	head->prev = head;
}

static inline void __list_add(struct list_head *new,
                              struct list_head *prev,
                              struct list_head *next) {
        next->prev = new;
        new->next = next;
        new->prev = prev;
        prev->next = new;
}

void list_add_tail(struct list_head *new, struct list_head *head) {
	__list_add(new, head, head->next);
}

void list_del(struct list_head *entry) {
}
