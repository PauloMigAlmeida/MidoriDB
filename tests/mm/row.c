/*
 * table.c
 *
 *  Created on: 7/03/2023
 *      Author: paulo
 */

#include "tests/mm.h"
#include "mm/table.h"

static size_t count_datablocks(struct table *table)
{
	struct list_head *pos;
	size_t ret = 0;
	list_for_each(pos, table->datablock_head)
	{
		ret++;
	}
	return ret;
}

static bool check_row_data(struct table *table, size_t row_num, void *expected, size_t len)
{
	printf("check_row_data: row_num: %zu\n", row_num);
	//TODO implement jump to next datablock when EOB is found
	struct list_head *pos;
	size_t i = 0;
	list_for_each(pos, table->datablock_head)
	{
		struct datablock *block = list_entry(pos, typeof(*block), head);

		for (size_t j = 0; j < ARR_SIZE(block->data) / struct_size_const(struct row, data, len); j++) {
			struct row *row = (struct row*)&block->data[j * (sizeof(struct row) + len)];
			if (i == row_num)
				return memcmp(row->data, expected, len) == 0;
			i++;
		}
	}

	return false;
}

static void create_test_table(struct table **out, size_t column_count)
{
	struct table *table;
	struct column column;

	table = table_init("test");

	for (size_t i = 0; i < column_count; i++) {
		snprintf(column.name, TABLE_MAX_COLUMN_NAME, "column_%zu", i);
		column.type = INTEGER;
		column.precision = sizeof(int);
		CU_ASSERT(table_add_column(table, &column));
		CU_ASSERT_EQUAL(table->column_count, i + 1);
	}

	*out = table;
}

void test_table_insert_row(void)
{
	struct table *table;
	int data[] = {1, 2, 3};
	size_t fit_in_datablock = (DATABLOCK_PAGE_SIZE / (struct_size_const(struct row, data, sizeof(data))));

	/* valid case - normal case */
	create_test_table(&table,ARR_SIZE(data));
	CU_ASSERT(table_insert_row(table, data, sizeof(data)));
	CU_ASSERT_EQUAL(count_datablocks(table), 1);
	CU_ASSERT(check_row_data(table, 0, data, sizeof(data)));
	CU_ASSERT(table_destroy(&table));

	/* valid case - force allocate new datablocks */
	create_test_table(&table,ARR_SIZE(data));

	for (size_t i = 0; i < fit_in_datablock * 3; i++) {
		CU_ASSERT(table_insert_row(table, data, sizeof(data)));
		CU_ASSERT(check_row_data(table, i, data, sizeof(data)));
	}
	CU_ASSERT_EQUAL(count_datablocks(table), 3);
	CU_ASSERT(table_destroy(&table));
}
