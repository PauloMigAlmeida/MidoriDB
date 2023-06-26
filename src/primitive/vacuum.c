/*
 * vacuum.c
 *
 *  Created on: 8/04/2023
 *      Author: paulo
 */

#include <primitive/table.h>
#include <primitive/column.h>
#include <primitive/row.h>

bool table_vacuum(struct table *table)
{
	struct list_head *dst_pos, *src_pos, *src_tmp_pos;
	struct datablock *dst_entry, *src_entry;
	size_t dst_blk_offset, dst_blk_idx;
	size_t src_blk_offset, src_blk_idx;
	struct row *row;
	size_t row_size;
	bool dst_full;

	/* sanity checks */
	if (!table)
		return false;

	dst_blk_idx = 0;
	src_blk_idx = 0;
	dst_blk_offset = 0;
	src_blk_offset = 0;
	dst_entry = NULL;
	src_entry = NULL;
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

					/* do we need to move the row at all? (common on first datablock) */
					if (dst_pos == src_pos && dst_blk_offset == i * row_size) {
						dst_blk_offset += row_size;
						continue;
					}

					/* free any variable precision resources in old row. (if any) */
					table_free_row_content(table, (struct row*)&dst_entry->data[dst_blk_offset]);

					/* good to go :) */
					memmove(&dst_entry->data[dst_blk_offset], row, row_size);
					dst_blk_offset += row_size;

					/* zero-out row content so we keep things tidy */
					memzero(row, row_size);
					row->flags.empty = true;
					row->flags.deleted = false;
				}

				src_blk_offset += row_size;
			}

			if (!dst_full) {
				src_blk_idx++;
				tmp_cnt++;
				src_blk_offset = 0;
			} else {
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

	/* turn remaining space of last non-free datablock into empty rows */
	for (size_t i = dst_blk_offset / row_size; i < DATABLOCK_PAGE_SIZE / row_size; i++) {
		table_free_row_content(table, (struct row*)&dst_entry->data[i * row_size]);
	}
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

		/* free any variable precision resources in old rows. (if any) */
		for (size_t i = 0; i < DATABLOCK_PAGE_SIZE / row_size; i++) {
			table_free_row_content(table, (struct row*)&src_entry->data[i * row_size]);
		}

		datablock_free(src_entry);
	}

	return true;
}
