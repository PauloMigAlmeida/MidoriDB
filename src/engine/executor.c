/*
 * executor.c
 *
 *  Created on: 8/06/2023
 *      Author: paulo
 */

#include <engine/executor.h>
#include <primitive/table.h>

#include <datastructure/linkedlist.h>

void add_index(struct ast_index_def_node *idx_def_node, struct table *table)
{
	struct list_head *pos;
	struct ast_index_column_node *entry;
	struct column *col;

	list_for_each(pos, idx_def_node->node_children_head)
	{
		entry = list_entry(pos, typeof(*entry), head);

		for (int i = 0; i < table->column_count; i++) {
			col = &table->columns[i];

			if (strncmp(col->name, entry->name, sizeof(col->name)) == 0) {
				col->indexed = idx_def_node->is_index;
				//TODO implement handler idx_def_node->is_pk
			}
		}
	}
}

int add_column(struct ast_column_def_node *col_def_node, struct table *table)
{
	struct column column = {0};
	int rc = MIDORIDB_OK;

	strncpy(column.name, col_def_node->name, sizeof(column.name));
	column.type = col_def_node->type;
	column.precision = col_def_node->precision;

	if (!table_add_column(table, &column))
		rc = -MIDORIDB_INTERNAL;

	return rc;
}

int run_create_stmt(struct database *db, struct ast_create_node *create_node, struct query_output *output)
{
	struct list_head *pos;
	struct ast_node *entry;
	struct table *table;
	int rc = MIDORIDB_OK;

	table = table_init(create_node->table_name);

	if (!table) {
		rc = -MIDORIDB_INTERNAL;
		goto err;
	}

	list_for_each(pos, create_node->node_children_head)
	{
		entry = list_entry(pos, typeof(*entry), head);

		if (entry->node_type == AST_TYPE_COLUMNDEF) {
			if ((rc = add_column((struct ast_column_def_node*)entry, table)))
				goto err;
		} else if (entry->node_type == AST_TYPE_INDEXDEF) {
			add_index((struct ast_index_def_node*)entry, table);
		}
	}

	rc = database_add_table(db, table);

	err:
	return rc;
}

int executor_run(struct database *db, struct ast_node *node, struct query_output *output)
{
	/* sanity checks */
	BUG_ON(!db || !node || !output);

	if (node->node_type == AST_TYPE_CREATE)
		return run_create_stmt(db, (struct ast_create_node*)node, output);
	else
		/* semantic analysis not implemented for that yet */
		BUG_ON(true);

	return -MIDORIDB_INTERNAL;
}
