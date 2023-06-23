/*
 * executor.c
 *
 *  Created on: 8/06/2023
 *      Author: paulo
 */

#include <engine/executor.h>
#include <primitive/table.h>

#include <datastructure/linkedlist.h>

static void add_index(struct ast_crt_index_def_node *idx_def_node, struct table *table)
{
	struct list_head *pos;
	struct ast_crt_index_column_node *entry;
	struct column *col;

	list_for_each(pos, idx_def_node->node_children_head)
	{
		entry = list_entry(pos, typeof(*entry), head);

		for (int i = 0; i < table->column_count; i++) {
			col = &table->columns[i];

			if (strncmp(col->name, entry->name, sizeof(col->name)) == 0) {
				col->indexed = idx_def_node->is_index;

				/*
				 * When primary key is defined after the column definition,
				 * the AST doesn't get the change to tweak the other attributes
				 * which change as a side effect. So we need to do it here
				 *
				 * See more at: ast_create.c
				 */
				if (idx_def_node->is_pk) {
					col->primary_key = true;
					col->nullable = false;
					col->unique = true;
				}
			}
		}
	}
}

static int add_column(struct ast_crt_column_def_node *col_def_node, struct table *table)
{
	struct column column = {0};
	int rc = MIDORIDB_OK;

	strncpy(column.name, col_def_node->name, sizeof(column.name));
	column.type = col_def_node->type;
	column.precision = col_def_node->precision;
	column.nullable = col_def_node->attr_null;
	column.unique = col_def_node->attr_uniq_key;
	column.auto_inc = col_def_node->attr_auto_inc;

	/* this can be overridden still if PK is declared after column definition */
	column.primary_key = col_def_node->attr_prim_key;

	if (!table_add_column(table, &column))
		rc = -MIDORIDB_INTERNAL;

	return rc;
}

static int run_create_stmt(struct database *db, struct ast_crt_create_node *create_node, struct query_output *output)
{
	struct list_head *pos;
	struct ast_node *entry;
	struct table *table;
	int rc = MIDORIDB_OK;

	/* evaluate the "IF NOT EXISTS" option, leave early if so */
	if (create_node->if_not_exists && database_table_exists(db, create_node->table_name)) {
		return rc;
	}

	table = table_init(create_node->table_name);

	if (!table) {
		rc = -MIDORIDB_INTERNAL;
		goto err;
	}

	list_for_each(pos, create_node->node_children_head)
	{
		entry = list_entry(pos, typeof(*entry), head);

		if (entry->node_type == AST_TYPE_CRT_COLUMNDEF) {
			if ((rc = add_column((struct ast_crt_column_def_node*)entry, table)))
				goto err;
		} else if (entry->node_type == AST_TYPE_CRT_INDEXDEF) {
			add_index((struct ast_crt_index_def_node*)entry, table);
		}
	}

	rc = database_table_add(db, table);

err:
	return rc;
}

int executor_run(struct database *db, struct ast_node *node, struct query_output *output)
{
	/* sanity checks */
	BUG_ON(!db || !node || !output);

	if (node->node_type == AST_TYPE_CRT_CREATE)
		return run_create_stmt(db, (struct ast_crt_create_node*)node, output);
	else
		/* semantic analysis not implemented for that yet */
		BUG_ON(true);

	return -MIDORIDB_INTERNAL;
}
