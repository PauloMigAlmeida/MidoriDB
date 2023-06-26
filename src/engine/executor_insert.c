/*
 * executor_insert.c
 *
 *  Created on: 27/06/2023
 *	Author: paulo
 */

#include <engine/executor.h>
#include <primitive/table.h>
#include <datastructure/linkedlist.h>


int executor_run_insertvals_stmt(struct database *db, struct ast_ins_insvals_node *ins_node, struct query_output *output)
{
	struct list_head *pos;
	struct ast_node *entry;
	struct table *table;
	int rc = MIDORIDB_OK;

	/*
	 * lock the database until we get a reference to the table and invoke table_lock()
	 * that way no other thread can mess with this table for now.
	 */
	if ((rc = database_lock(db)))
		goto err;

	if (!database_table_exists(db, ins_node->table_name)) {
		rc = -MIDORIDB_INTERNAL;
		goto err_tbl_lookup;
	}

	table = database_table_get(db, ins_node->table_name);

	if ((rc = table_lock(table)))
		goto err_tbl_lock;

	/* we don't need the lock on the entire db for that anymore. Let it be used by other thread if any */
	database_unlock(db);

	//TODO implement handler for when opt_columnlist is specified

	list_for_each(pos, ins_node->node_children_head)
	{
		entry = list_entry(pos, typeof(*entry), head);

		if (entry->node_type == AST_TYPE_INS_VALUES) {

		}

//		table_insert_row(table, row, len);
	}
	output->n_rows_aff = ins_node->row_count;

	/* clean up */
	table_unlock(table);

	return rc;

err_tbl_lock:
err_tbl_lookup:
	database_unlock(db);
err:
	return rc;
}
