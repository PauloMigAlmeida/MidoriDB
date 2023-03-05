#include "mm/datablock.h"

static uint64_t block_id_acc;

struct list_head* datablock_init(void)
{
	struct list_head *ret = NULL;
	if ((ret = zalloc(sizeof(*ret)))) {
		block_id_acc = 0;
		list_head_init(ret);
	}
	return ret;
}

struct datablock* datablock_alloc(struct list_head *head)
{
	struct datablock *new = NULL;
	if ((new = zalloc(sizeof(*new)))) {
		new->block_id = block_id_acc++;
		list_head_init(&new->head);
		list_add(&new->head, head->prev);
	}
	return new;
}

void datablock_free(struct datablock *block)
{
	list_del(&block->head);
	free(block);
}
