#include "mm/datablock.h"

static uint64_t block_id_acc;

void datablock_init(void) {
	if ((datablocks_head = malloc(sizeof(*datablocks_head)))) {
		list_head_init(datablocks_head);
		block_id_acc = 0;
	} else {
		die("couldn't initiate datablock");
	}
}

struct datablock *datablock_alloc(void) {
	struct datablock *new;
	if((new = malloc(sizeof(*new)))) {		
		memset(new, 0, sizeof(*new));
		new->block_id = block_id_acc++;
		list_head_init(&new->head);
		list_add(&new->head, datablocks_head->prev);
	} else {
		die("couldn't alloc datablock");
	}
	return new;
}

void datablock_free(struct datablock *block) {
	list_del(&block->head);
	free(block);
}
