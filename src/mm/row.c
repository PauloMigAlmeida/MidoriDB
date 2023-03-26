/*
 * row.c
 *
 *  Created on: 17/03/2023
 *      Author: paulo
 */
#include "mm/table.h"

size_t table_calc_row_data_size(struct table *table)
{
	size_t size = 0;
	for (int i = 0; i < table->column_count; i++) {
		/* for variable length column we store a pointer to the data */
		if (table_check_var_column(&table->columns[i]))
			size += sizeof(uintptr_t);
		else
			size += table->columns[i].precision;
	}

	return size;
}

size_t table_calc_row_size(struct table *table)
{
	return struct_size_const(struct row, data, table_calc_row_data_size(table));
}

static void table_datablock_init(struct datablock *block, size_t row_size)
{
	for (size_t i = 0; i < (DATABLOCK_PAGE_SIZE / row_size); i++) {
		struct row *row = (struct row*)&block->data[row_size * i];
		row->header.empty = true;
		row->header.deleted = false;
	}
}

bool table_insert_row(struct table *table, struct row *row, size_t len)
{
	struct datablock *block;
	bool should_alloc;

	/* sanity checks */
	if (!table || !row || len == 0 || len != table_calc_row_size(table))
		return false;

	BUG_ON(row->header.empty || row->header.deleted);

	/* is this the first ever item of the table ? */
	should_alloc = table->datablock_head == table->datablock_head->prev;
	/* or is there enough space to insert that into an existing datablock ? */
	should_alloc = should_alloc || (table->free_dtbkl_offset + len) >= DATABLOCK_PAGE_SIZE;
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
	memcpy(&new_row->header, &row->header, sizeof(row->header));

	/*
	 * if any column has a variable precision type then we also have to alloc memory to hold the content
	 * while we only store the pointer into the row / datablock.
	 */
	size_t pos = 0;
	int column_idx;
	for (column_idx = 0; column_idx < table->column_count; column_idx++) {
		struct column *column = &table->columns[column_idx];
		if (table_check_var_column(column)) {
			void *ptr = zalloc(column->precision);
			
			if(!ptr)
				goto err;

			// This line will give me a headache... I'm sure of it
			memcpy(ptr, *((char**)((char*)row->data + pos)), column->precision);

			uintptr_t *col_idx_ptr = (uintptr_t*)&new_row->data[pos];
			*col_idx_ptr = (uintptr_t)ptr;

			pos += sizeof(uintptr_t);
		} else {
			memcpy(new_row->data + pos, ((char*)row->data) + pos, column->precision);
			pos += column->precision;
		}

	}

	table->free_dtbkl_offset += len;

	return true;

err:

/* 
 * if we failed to alloc memory for variable precision columns then 
 * we have to free those we got so far to avoid memory leak.
 * 
 * notes to myself:
 * 	- I need to figure out a way to simulate this via valgrind so I 
 * 		can properly unit test this routine.
*/
pos = 0;
for (int i = 0; i < column_idx; i++){
	struct column *column = &table->columns[column_idx];
	if (table_check_var_column(column)) {
		uintptr_t **col_idx_ptr = (uintptr_t**)&new_row->data[pos];
		free(*col_idx_ptr);
		pos += sizeof(uintptr_t);
	} else {
		pos += column->precision;
	}
}

memzero(new_row->data, table_calc_row_size(table));
new_row->header.empty = true;
new_row->header.deleted = false;

return false;
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

bool table_update_row(struct table *table, struct datablock *blk, size_t offset, void *data, size_t len)
{
	/* sanity checks */
	if (!table || !blk || offset >= DATABLOCK_PAGE_SIZE || len != table_calc_row_data_size(table))
		return false;

	struct row *row = (struct row*)&blk->data[offset];

	/* something went terribly wrong here if this is true */
	BUG_ON(row->header.deleted || row->header.empty);

	size_t pos = 0;
	for (int i = 0; i < table->column_count; i++) {
		struct column *column = &table->columns[i];
		if (table_check_var_column(column)) {
			void **ptr = (void**)&row->data[pos];
			memcpy(*ptr, *((char**)((char*)data + pos)), column->precision);
			pos += sizeof(uintptr_t);
		} else {
			memcpy(row->data + pos, ((char*)data) + pos, column->precision);
			pos += column->precision;
		}

	}

	return true;
}

