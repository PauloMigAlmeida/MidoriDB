/*
 * executor_insert.c
 *
 *  Created on: 27/06/2023
 *	Author: paulo
 */

#define _XOPEN_SOURCE       /* See feature_test_macros(7) */
#include <engine/executor.h>
#include <primitive/table.h>
#include <primitive/column.h>
#include <primitive/row.h>
#include <datastructure/linkedlist.h>
#include <lib/bit.h>

static int parse_date_type(struct ast_ins_exprval_node *exprval_node, enum COLUMN_TYPE type,
		struct query_output *output,
		time_t *time_out
		)
{
	int rc = MIDORIDB_OK;
	struct tm time_struct = {0};
	const char *fmt;

	if (type == CT_DATE)
		fmt = COLUMN_CTDATE_FMT;
	else
		fmt = COLUMN_CTDATETIME_FMT;

	// Parse the string into a time structure
	if (strptime(exprval_node->str_val, fmt, &time_struct) == NULL)
		goto err_parse;

	// Convert the time structure to a time_t value
	*time_out = mktime(&time_struct);

	// Check if the conversion was successful
	if (*time_out == -1)
		goto err_convert;

	return rc;

err_parse:
	rc = -MIDORIDB_ERROR;
	snprintf(output->error.message, sizeof(output->error.message) - 1,
			/* max str size would exceed buffer's size, so trim it */
			"Failed to parse the time string: '%.256s...'\n",
			exprval_node->str_val);
	return rc;

err_convert:
	rc = -MIDORIDB_ERROR;
	snprintf(output->error.message, sizeof(output->error.message) - 1,
			/* max str size would exceed buffer's size, so trim it */
			"Failed to convert the time structure to time_t: '%.256s...'\n",
			exprval_node->str_val);
	return rc;
}

static struct ast_ins_exprval_node* eval_expr(struct ast_node *node)
{
	/*
	 * TODO move this to the optimisation phase.
	 * I want to keep the execution phase as simple as possible.
	 */
	UNUSED(node);
	return NULL;
}

static int build_row(struct table *table,
		struct ast_ins_values_node *vals_node,
		int *column_order,
		struct row *row_out,
		struct query_output *output)
{
	struct list_head *pos;
	struct ast_node *entry;
	struct ast_ins_exprval_node *exprval_entry;
	int val_pos = 0;
	size_t row_pos = 0;
	struct column *column;
	size_t col_space;
	time_t dt = 0;
	int rc = MIDORIDB_OK;

	/* initialise row flags */
	row_out->flags.deleted = false;
	row_out->flags.empty = false;

	/*
	 * setting all fields to NULL. If value is specified then we unset the bit for that column.
	 * if field still null at the end then either the value was a explicit NULL or the column
	 * has been left out of the INSERT stmt via the opt_column_list
	 */
	for (int i = 0; i < table->column_count; i++) {
		bit_set(row_out->null_bitmap, i, sizeof(row_out->null_bitmap));
	}

	list_for_each(pos, vals_node->node_children_head)
	{
		entry = list_entry(pos, typeof(*entry), head);
		column = &table->columns[column_order[val_pos]];
		col_space = table_calc_column_space(column);
		row_pos = 0;

		/* calculate column displacement in row's payload, then again user can fiddle with opt_column_list */
		for (int i = 0; i < column_order[val_pos]; i++) {
			row_pos += table_calc_column_space(&table->columns[i]);
		}

		// TODO I will possibly move this to the optimisation phase
		if (entry->node_type == AST_TYPE_INS_EXPROP) {
			exprval_entry = eval_expr(entry);
			//TODO implement error handling
		}

		exprval_entry = (struct ast_ins_exprval_node*)entry;

		if (exprval_entry->is_intnum) {
			memcpy(row_out->data + row_pos, &exprval_entry->int_val, col_space);
		} else if (exprval_entry->is_approxnum) {
			memcpy(row_out->data + row_pos, &exprval_entry->double_val, col_space);
		} else if (exprval_entry->is_bool) {
			memcpy(row_out->data + row_pos, &exprval_entry->bool_val, col_space);
		} else if (exprval_entry->is_null) {
			/*
			 * I don't have to zero out part of the row payload for null value
			 * but that makes troubleshooting slightly easier
			 */
			memzero(row_out->data + row_pos, col_space);
		} else if (exprval_entry->is_str) {

			/*
			 * okay, we know that the value is a string, the question is whether this value should
			 * be interpreted in any other way such as DATE, DATETIME (and possibly others in the future)
			 */

			if (column->type == CT_DATE || column->type == CT_DATETIME) {

				if ((rc = parse_date_type(exprval_entry, column->type, output, &dt)))
					goto err;

				memcpy(row_out->data + row_pos, &dt, col_space);

			} else {
				BUG_ON_CUSTOM_MSG(true, "not implemented yet");
				/* note to myself: VARCHAR() precision is NUL-char inclusive */
				char *tmp_str = zalloc(column->precision);
				if (!tmp_str) {
					rc = -MIDORIDB_INTERNAL;
					goto err;
				}
				strncpy(tmp_str, exprval_entry->str_val, column->precision - 1);
				memcpy(row_out->data + row_pos, &tmp_str, col_space);
			}

		} else {
			BUG_ON_CUSTOM_MSG(true, "not implemented yet");
		}

		if (!exprval_entry->is_null)
			bit_clear(row_out->null_bitmap, column_order[val_pos], sizeof(row_out->null_bitmap));

		val_pos++;
	}

	return rc;

err:
	return rc;
}

/* columns can be specified in an order that's different from the order defined in the CREATE stmt */
static void build_column_order(struct table *table, struct ast_ins_insvals_node *insvals_node, int *column_order, int len)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_ins_inscols_node *inscols_node;
	struct ast_ins_column_node *col_node;
	int opt_col_list_idx;

	if (!insvals_node->opt_column_list) {
		/* column list wasn't specified, so it follows the natural order defined in the CREATE stmt */
		for (int i = 0; i < table->column_count; i++)
			column_order[i] = i;
	} else {
		/* column list was defined, so we create a mapping of that order */

		// find AST_TYPE_INS_INSCOLS node
		list_for_each(pos, insvals_node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);
			if (tmp_entry->node_type == AST_TYPE_INS_INSCOLS) {
				inscols_node = (struct ast_ins_inscols_node*)tmp_entry;
				break;
			}
		}

		/* sanity check */
		BUG_ON(!inscols_node);

		opt_col_list_idx = 0;
		list_for_each(pos, inscols_node->node_children_head)
		{
			col_node = list_entry(pos, typeof(*col_node), head);

			BUG_ON(len < table->column_count);

			for (int i = 0; i < table->column_count; i++) {
				struct column *col = &table->columns[i];

				if (strncmp(col->name, col_node->name, sizeof(col->name)) == 0) {
					column_order[opt_col_list_idx] = i;
					break;
				}
			}
			opt_col_list_idx++;
		}
	}
}

int executor_run_insertvals_stmt(struct database *db, struct ast_ins_insvals_node *ins_node,
		struct query_output *output)
{
	struct list_head *pos;
	struct ast_node *entry;
	struct table *table;
	struct row *row;
	int column_order[TABLE_MAX_COLUMNS];
	int rc = MIDORIDB_OK;

	if (!database_table_exists(db, ins_node->table_name)) {
		rc = -MIDORIDB_INTERNAL;
		goto err;
	}

	table = database_table_get(db, ins_node->table_name);

	memset(column_order, -1, sizeof(column_order));

	/* columns can be specified in an order that's different from the order defined in the CREATE stmt */
	build_column_order(table, ins_node, column_order, ARR_SIZE(column_order));

	list_for_each(pos, ins_node->node_children_head)
	{
		entry = list_entry(pos, typeof(*entry), head);

		if (entry->node_type == AST_TYPE_INS_VALUES) {

			row = zalloc(table_calc_row_size(table));

			if (!row)
				goto err_row_alloc;

			if ((rc = build_row(table, (struct ast_ins_values_node*)entry, column_order, row, output)))
				goto err_build_row;

			if (!table_insert_row(table, row, table_calc_row_size(table))) {
				rc = -MIDORIDB_INTERNAL;
				goto err_ins_row;
			}

			table_free_row_content(table, row);
			free(row);
		}
	}

	output->n_rows_aff = ins_node->row_count;

	return rc;

err_ins_row:
err_build_row:
	free(row);
err_row_alloc:
err:
	return rc;
}
