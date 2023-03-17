/*
 * row.c
 *
 *  Created on: 17/03/2023
 *      Author: paulo
 */
#include "mm/table.h"

size_t table_calc_row_size(struct table *table)
{
	size_t size = 0;
	for (int i = 0; i < table->column_count; i++)
		size += table->columns[i].precision;
	return size;
}

static void table_datablock_init(struct datablock *block, size_t row_size)
{
	for (size_t i = 0; i < (DATABLOCK_PAGE_SIZE / row_size); i++) {
		struct row *row = (struct row*)&block->data[row_size * i];
		row->header.empty = true;
		row->header.deleted = false;
	}
}

bool table_insert_row(struct table *table, void *data, size_t len)
{
	struct datablock *block;
	bool should_alloc;

	/* sanity checks */
	if (!table || !data || len == 0)
		return false;

	/*
	 * check whether the data we are trying to insert is beyond what this table is
	 * meant hold - most likely someone has made a mistake for that to be the case.
	 * Then again, this is C.. you can never be "too safe" under any circumstance
	 */
	if (len != table_calc_row_size(table))
		return false;

	/* is this the first ever item of the table ? */
	should_alloc = table->datablock_head == table->datablock_head->prev;
	/* or is there enough space to insert that into an existing datablock ? */
	should_alloc = should_alloc || (table->free_dtbkl_offset +
			struct_size_const(struct row, data, len)) >= DATABLOCK_PAGE_SIZE;
	if (should_alloc) {
		// Notes to myself, paulo, you should test the crap out of that..
		// TODO add some sort of POISON/EOF so when reading the datablock
		// we would know that it's time to  go to the next datablock
		//
		// We also have to differentiate between EOF and data deleted.. this will
		// happen once we implement delete operations
		if (!(block = datablock_alloc(table->datablock_head)))
			return false;

		table_datablock_init(block, len);
		table->free_dtbkl_offset = 0;
	} else {
		// since it's a circular linked list then getting the head->prev is the same
		// as getting the last available data block
		block = list_entry(table->datablock_head->prev, typeof(*block), head);
	}

	struct row *new_row = (struct row*)&block->data[table->free_dtbkl_offset];
	new_row->header.empty = false;
	new_row->header.deleted = false;
	memcpy(new_row->data, data, len);

	table->free_dtbkl_offset += struct_size(new_row, data, len);

	return true;
}

bool table_delete_row(struct table *table, struct datablock *blk, size_t offset)
{
	/* sanity checks */
	if (!table || !blk || offset >= DATABLOCK_PAGE_SIZE)
		return false;

	struct row *row = (struct row*)&blk->data[offset];

	/* something went terribly wrong here if this is true */
	BUG_ON(row->header.deleted || row->header.empty);

	row->header.deleted = true;
	return true;
}

