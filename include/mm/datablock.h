#ifndef MM_DATABLOCK_H
#define MM_DATABLOCK_H

#include "compiler/common.h"
#include "datastructure/linkedlist.h"

#define DATABLOCK_PAGE_SIZE	4096

struct list_head *datablocks_head;

struct datablock {
	uint64_t block_id;
	char data[DATABLOCK_PAGE_SIZE];
	struct list_head head;
};

void datablock_init(void);
struct datablock *datablock_alloc(void);
void datablock_free(struct datablock *block);

#endif /* MM_DATABLOCK_H */
