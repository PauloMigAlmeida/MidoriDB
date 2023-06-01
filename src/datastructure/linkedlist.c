#include "datastructure/linkedlist.h"

void list_head_init(struct list_head *head)
{
	head->next = head;
	head->prev = head;
}

void list_add(struct list_head *new, struct list_head *head)
{
	head->next->prev = new;
	new->next = head->next;
	new->prev = head;
	head->next = new;
}

void list_del(struct list_head *entry)
{
	entry->prev->next = entry->next;
	entry->next->prev = entry->prev;
	entry->prev = NULL;
	entry->next = NULL;
}

size_t list_length(struct list_head *head)
{
	size_t count = 0;
	struct list_head *pos = NULL;
	list_for_each(pos, head)
	{
		count++;
	}
	return count;
}
