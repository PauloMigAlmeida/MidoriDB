/*
 * vacuum.c
 *
 *  Created on: 8/04/2023
 *      Author: paulo
 */

#include <primitive/table.h>

bool table_vacuum(struct table *table)
{
	struct list_head *dst_pos, *src_pos, *src_tmp_pos;
	struct datablock *dst_entry = NULL, *src_entry;
	size_t dst_blk_offset = 0, dst_blk_idx;
	size_t src_blk_offset, src_blk_idx;
	struct row *row;
	size_t row_size;

	/* sanity checks */
	if (!table)
		return false;

	if (pthread_mutex_lock(&table->mutex))
		return false;

	bool dst_full;
	dst_blk_idx = 0;
	src_blk_idx = 0;

	src_blk_offset = 0;
	row_size = table_calc_row_size(table);

	list_for_each(dst_pos, table->datablock_head)
	{
		dst_entry = list_entry(dst_pos, typeof(*dst_entry), head);
		dst_blk_offset = 0;
		dst_full = false;

		size_t tmp_cnt = 0;
		list_for_each(src_pos, table->datablock_head)
		{
			/* skip to the right datablock if needed */
			if (tmp_cnt < src_blk_idx) {
				tmp_cnt++;
				continue;
			}

			src_entry = list_entry(src_pos, typeof(*src_entry), head);

			for (size_t i = src_blk_offset / row_size; i < DATABLOCK_PAGE_SIZE / row_size; i++) {

				row = (struct row*)&src_entry->data[i * row_size];

				/* is this valid row ? */
				if (!row->flags.deleted && !row->flags.empty) {
//					printf("i = %lu\n", (src_blk_idx * (DATABLOCK_PAGE_SIZE / row_size)) + i);
					/* can this row still fit in the dst datablock ? */
					if (dst_blk_offset + row_size >= DATABLOCK_PAGE_SIZE) {
						dst_full = true;
						break;
					}

					memmove(&dst_entry->data[dst_blk_offset], row, row_size);
					dst_blk_offset += row_size;
				}

				src_blk_offset += row_size;
			}

			if (!dst_full) {
				src_blk_idx++;
				tmp_cnt++;
				src_blk_offset = 0;
			}else{
				break;
			}

		}

		dst_blk_idx++;

		/* have we copied everything that we needed? */
		if (!dst_full) {
			break;
		}
	}

	/* adjust offset to next available row */
	table->free_dtbkl_offset = dst_blk_offset;

	/* turn remaining space into empty rows */
	table_datablock_init(dst_entry, dst_blk_offset, row_size);

	/* free blocks that are no longer used */
	src_blk_idx = 0;
	list_for_each_safe(src_pos, src_tmp_pos, table->datablock_head)
	{
		/* skip to the right datablock if needed */
		if (src_blk_idx < dst_blk_idx) {
			src_blk_idx++;
			continue;
		}

		src_entry = list_entry(src_pos, typeof(*src_entry), head);

		datablock_free(src_entry);
	}

	if (pthread_mutex_unlock(&table->mutex))
		return false;

	return true;
}
