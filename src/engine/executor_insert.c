/*
 * executor_insert.c
 *
 *  Created on: 27/06/2023
 *	Author: paulo
 */

#include <engine/executor.h>
#include <primitive/table.h>
#include <primitive/column.h>
#include <primitive/row.h>
#include <datastructure/linkedlist.h>

static int validate_value_for_column(struct column *column,
				     struct ast_ins_exprval_node *vals_node,
				     struct query_output *output)
{
	int rc = MIDORIDB_OK;
	
	if (vals_node->is_str &&
	     !(column->type == CT_VARCHAR ||
	       column->type == CT_DATE ||
	       column->type == CT_DATETIME))
		goto err_type;

	if (vals_node->is_intnum && column->type != CT_INTEGER)
		goto err_type;

	if (vals_node->is_approxnum && column->type != CT_DOUBLE)
		goto err_type;

	//TODO add type for is_bool
	BUG_ON(vals_node->is_bool);

	return rc;

//TODO add error_handler for other types too... right now everything is a string
err_type:
	rc = -MIDORIDB_ERROR;
	snprintf(output->error.message, 
		 sizeof(output->error.message) - 1,
		 /* max str size would exceed buffer's size, so trim it */
		 "value: '%.256s...' is not suitable for column %d\n",
		 vals_node->str_val,
		 column->type);
	return rc;
}

static struct ast_ins_exprval_node* eval_expr(struct ast_node *node)
{
	UNUSED(node);
	return NULL;
}

static int build_row(struct table *table,
		     struct ast_ins_values_node *vals_node,
		     struct row *row_out,
		     struct query_output *output)
{
	struct list_head *pos;
	struct ast_node *entry;
	struct ast_ins_exprval_node *exprval_entry;
	int col_pos = 0;
	size_t row_pos = 0;
	struct column *column;
	int rc = MIDORIDB_OK;

	list_for_each(pos, vals_node->node_children_head)
	{
		entry = list_entry(pos, typeof(*entry), head);
		column = &table->columns[col_pos];

		if (entry->node_type == AST_TYPE_INS_EXPROP) {
			exprval_entry = eval_expr(entry);
			//TODO implement error handling
		}

		exprval_entry = (struct ast_ins_exprval_node*)entry;

		if ((rc = validate_value_for_column(column, exprval_entry, output)))
			goto err;
		
		if (exprval_entry->is_intnum) {
			memcpy(row_out->data + row_pos, &exprval_entry->int_val, sizeof(exprval_entry->int_val));
		} else if (exprval_entry->is_approxnum) {
			memcpy(row_out->data + row_pos, &exprval_entry->double_val, sizeof(exprval_entry->double_val));
		} else if (exprval_entry->is_bool) {
			memcpy(row_out->data + row_pos, &exprval_entry->bool_val, sizeof(exprval_entry->bool_val));
		} else {
			/* note for myself: VARCHAR() precision is NUL-char inclusive */
			char *tmp_str = zalloc(column->precision);
			if (!tmp_str) {
				rc = -MIDORIDB_INTERNAL;
				goto err;
			}
			strncpy(tmp_str, exprval_entry->str_val, column->precision - 1);
			row_out->data[row_pos] = (uintptr_t)tmp_str;
		}

		col_pos++;
		row_pos += table_calc_column_space(column);
	}

	return rc;

err:
	return rc;
}

int executor_run_insertvals_stmt(struct database *db, struct ast_ins_insvals_node *ins_node,
				 struct query_output *output)
{
	struct list_head *pos;
	struct ast_node *entry;
	struct table *table;
	struct row *row;
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

			row = zalloc(table_calc_row_size(table));

			if (!row)
				goto err_row_alloc;

			row->flags.deleted = false;
			row->flags.empty = false;
			
			if ((rc = build_row(table, (struct ast_ins_values_node*)entry, row, output)))
				goto err_build_row;

			if (!table_insert_row(table, row, table_calc_row_size(table))){
				rc = -MIDORIDB_INTERNAL;
				goto err_ins_row;
			}

			table_free_row_content(table, row);
			free(row);
		}
	}

	output->n_rows_aff = ins_node->row_count;

	/* clean up */
	table_unlock(table);

	return rc;

err_ins_row:
err_build_row:
	free(row);
err_row_alloc:
err_tbl_lock:
err_tbl_lookup:
	database_unlock(db);
err:
	return rc;
}
