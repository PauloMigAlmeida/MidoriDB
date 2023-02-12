#include "datastructure/linkedlist.h"

void list_head_init(struct list_head *head){
	head->next = head;
	head->prev = head;
}

void list_add_tail(struct list_head *new, struct list_head *head) {
}

void list_del(struct list_head *entry) {
}

