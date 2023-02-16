#include "tests/mm.h"
#include "mm/datablock.h"

void test_datablock_init(void) {
	datablock_init();
	CU_ASSERT(datablocks_head != NULL);
	CU_ASSERT(datablocks_head->prev == datablocks_head->next);
	free(datablocks_head);
}

void test_datablock_alloc(void) {
	struct datablock *block;
	datablock_init();
	block = datablock_alloc();
	CU_ASSERT(datablocks_head->next == &block->head);
	CU_ASSERT(block->head.next == datablocks_head);
	datablock_free(block);
	free(datablocks_head);
}

void test_datablock_free(void) {
	struct datablock *block;
	datablock_init();
	
	block = datablock_alloc();
	CU_ASSERT(datablocks_head->next == &block->head);
	CU_ASSERT(block->head.next == datablocks_head);
	
	datablock_free(block);
	CU_ASSERT(datablocks_head != NULL);
	CU_ASSERT(datablocks_head->prev == datablocks_head->next);

	free(datablocks_head);
}

void test_datablock_iterate(void) {
	struct datablock *block1, *block2, *block3;
	struct datablock *entry = NULL;
	struct list_head *pos = NULL;
	uint64_t sum_blk_ids = 0;
	
	datablock_init();
	block1 = datablock_alloc();
	block2 = datablock_alloc();
	block3 = datablock_alloc();

	list_for_each(pos, datablocks_head) {
		entry = list_entry(pos, typeof(*entry), head);
		sum_blk_ids += entry->block_id;
	}

	CU_ASSERT_EQUAL(sum_blk_ids, 3);

	datablock_free(block1);
	datablock_free(block2);
	datablock_free(block3);
	free(datablocks_head);
}
