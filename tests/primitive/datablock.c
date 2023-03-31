#include <primitive/datablock.h>
#include <tests/primitive.h>

void test_datablock_init(void)
{
	struct list_head *head;

	head = datablock_init();
	CU_ASSERT_PTR_NOT_EQUAL(head, NULL);
	CU_ASSERT_PTR_EQUAL(head->prev, head->next);
	free(head);
}

void test_datablock_alloc(void)
{
	struct list_head *head;
	struct datablock *block;

	head = datablock_init();
	block = datablock_alloc(head);
	CU_ASSERT_PTR_EQUAL(head->next, &block->head);
	CU_ASSERT_PTR_EQUAL(block->head.next, head);
	datablock_free(block);

	free(head);
}

void test_datablock_free(void)
{
	struct list_head *head;
	struct datablock *block;

	head = datablock_init();
	block = datablock_alloc(head);

	datablock_free(block);
	CU_ASSERT_PTR_NOT_EQUAL(head, NULL);
	CU_ASSERT_PTR_EQUAL(head->prev, head->next);

	free(head);
}

void test_datablock_iterate(void)
{
	struct list_head *head;
	struct datablock *block1, *block2, *block3;
	struct datablock *entry = NULL;
	struct list_head *pos = NULL;
	uint64_t sum_blk_ids = 0;

	head = datablock_init();
	block1 = datablock_alloc(head);
	block2 = datablock_alloc(head);
	block3 = datablock_alloc(head);

	list_for_each(pos, head)
	{
		entry = list_entry(pos, typeof(*entry), head);
		sum_blk_ids += entry->block_id;
	}

	CU_ASSERT_EQUAL(sum_blk_ids,
			block1->block_id + block2->block_id + block3->block_id);

	datablock_free(block1);
	datablock_free(block2);
	datablock_free(block3);
	free(head);
}
