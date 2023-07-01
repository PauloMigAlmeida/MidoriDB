/*
 * semantic_insert.c
 *
 *  Created on: 1/07/2023
 *      Author: paulo
 */

#define _XOPEN_SOURCE       /* See feature_test_macros(7) */
#include <parser/semantic.h>
#include <primitive/table.h>
#include <primitive/column.h>
#include <datastructure/hashtable.h>
#include <datastructure/linkedlist.h>

static void free_str_entries(struct hashtable *hashtable, const void *key, size_t klen,
		const void *value, size_t vlen, void *arg)
{
	struct hashtable_entry *entry = NULL;
	UNUSED(arg);
	UNUSED(value);
	UNUSED(vlen);

	entry = hashtable_remove(hashtable, key, klen);
	hashtable_free_entry(entry);
}

static bool validate_table(struct database *db, struct ast_ins_insvals_node *insvals_node, char *out_err, size_t out_err_len)
{
	if (!table_validate_name(insvals_node->table_name)) {
		snprintf(out_err, out_err_len, "table name '%s' is invalid\n", insvals_node->table_name);
		return false;
	}

	if (!database_table_exists(db, insvals_node->table_name)) {
		snprintf(out_err, out_err_len, "table name '%s' doesn't exist\n", insvals_node->table_name);
		return false;
	}

	return true;
}

static bool column_exists(struct database *db, struct ast_ins_insvals_node *insvals_node, struct ast_ins_column_node *col_node)
{
	struct table *table;

	table = database_table_get(db, insvals_node->table_name);

	for (int i = 0; i < table->column_count; i++) {
		if (strncmp(table->columns[i].name, col_node->name, sizeof(table->columns[i].name)) == 0) {
			return true;
		}
	}

	return false;
}

static bool validate_column_list(struct database *db, struct ast_ins_insvals_node *insvals_node, char *out_err, size_t out_err_len)
{
	struct hashtable ht = {0};
	struct list_head *pos1, *pos2;
	struct ast_node *tmp_entry;
	struct ast_ins_inscols_node *inscols_node;
	struct ast_ins_values_node *vals_node;
	struct ast_ins_column_node *col_node;

	if (!hashtable_init(&ht, &hashtable_str_compare, &hashtable_str_hash)) {
		snprintf(out_err, out_err_len, "semantic phase: internal error\n");
		goto err;
	}

	list_for_each(pos1, insvals_node->node_children_head)
	{
		tmp_entry = list_entry(pos1, typeof(*tmp_entry), head);
		if (tmp_entry->node_type == AST_TYPE_INS_INSCOLS) {
			inscols_node = (struct ast_ins_inscols_node*)tmp_entry;

			list_for_each(pos2, inscols_node->node_children_head)
			{
				col_node = list_entry(pos2, typeof(*col_node), head);

				if (hashtable_get(&ht, col_node->name, strlen(col_node->name) + 1)) {
					snprintf(out_err, out_err_len, "duplicate column name: '%s'\n",
							col_node->name);
					goto err_dup_col_name;
				}

				if (!column_exists(db, insvals_node, col_node)) {
					snprintf(out_err, out_err_len, "column name '%s' doesn't exist\n",
							col_node->name);
					goto err_column_name;
				}

				/*TODO maybe I should create a Set datastructure -
				 * that would make this code easier on the eyes*/
				if (!hashtable_put(&ht, col_node->name, strlen(col_node->name) + 1,
							col_node->name,
							strlen(col_node->name) + 1)) {

					snprintf(out_err, out_err_len, "semantic phase: internal error\n");
					goto err_ht_put_col;
				}
			}
		}
	}

	/* validate if number of values specified match columns specified */
	list_for_each(pos1, insvals_node->node_children_head)
	{
		tmp_entry = list_entry(pos1, typeof(*tmp_entry), head);
		if (tmp_entry->node_type == AST_TYPE_INS_VALUES) {
			vals_node = (struct ast_ins_values_node*)tmp_entry;

			if (vals_node->value_count != (int)ht.count) {
				snprintf(out_err, out_err_len, "%d values for %lu columns\n",
						vals_node->value_count,
						ht.count);
				goto err_col_val_diff_num;
			}
		}
	}

	/* clean up */
	hashtable_foreach(&ht, &free_str_entries, NULL);
	hashtable_free(&ht);

	return true;

err_col_val_diff_num:
err_ht_put_col:
err_column_name:
err_dup_col_name:
	hashtable_foreach(&ht, &free_str_entries, NULL);
	hashtable_free(&ht);
err:
	return false;
}

static bool validate_number_terms(struct ast_ins_insvals_node *insvals_node, char *out_err, size_t out_err_len)
{
	int num_terms = -1;
	struct list_head *pos1;
	struct ast_node *tmp_entry;
	struct ast_ins_values_node *vals_node;

	list_for_each(pos1, insvals_node->node_children_head)
	{
		tmp_entry = list_entry(pos1, typeof(*tmp_entry), head);
		if (tmp_entry->node_type == AST_TYPE_INS_VALUES) {
			vals_node = (struct ast_ins_values_node*)tmp_entry;

			if (num_terms == -1) {
				// first time
				num_terms = vals_node->value_count;
			} else if (num_terms != vals_node->value_count) {
				// second time onwards
				snprintf(out_err, out_err_len, "all VALUES must have the same number of terms\n");
				return false;
			}
		}
	}

	return true;
}

static bool try_parse_date_type(char *str, enum COLUMN_TYPE type)
{
	struct tm time_struct = {0};
	const char *fmt;
	time_t time_out;

	if (type == CT_DATE)
		fmt = COLUMN_CTDATE_FMT;
	else
		fmt = COLUMN_CTDATETIME_FMT;

	// Parse the string into a time structure
	if (strptime(str, fmt, &time_struct) == NULL)
		return false;

	// Convert the time structure to a time_t value
	time_out = mktime(&time_struct);

	// Check if the conversion was successful
	if (time_out == -1)
		return false;

	return true;

}

static bool check_value_for_column(struct column *column, struct ast_node *node, char *out_err, size_t out_err_len)
{
	struct ast_ins_exprval_node *vals_node;

	if (node->node_type == AST_TYPE_INS_EXPRVAL) {
		vals_node = (struct ast_ins_exprval_node*)node;

		if (vals_node->is_str) {
			// strings can be interpreted/parsed for other types such as DATE and DATETIME
			if (column->type == CT_DATE || column->type == CT_DATETIME) {
				if (!try_parse_date_type(vals_node->str_val, column->type)) {
					snprintf(out_err, out_err_len,
							/* max str size would exceed buffer's size, so trim it */
							"val: '%.256s' can't be parsed for DATE | DATETIME column\n",
							vals_node->str_val);
					return false;
				}
			} else if (column->type != CT_VARCHAR) {
				snprintf(out_err, out_err_len,
						/* max str size would exceed buffer's size, so trim it */
						"val: '%.256s' requires an VARCHAR() column\n",
						vals_node->str_val);
				return false;
			}
		}

		if (vals_node->is_intnum && column->type != CT_INTEGER) {
			snprintf(out_err, out_err_len, "val: '%ld' requires an INTEGER column\n", vals_node->int_val);
			return false;
		}

		if (vals_node->is_approxnum && column->type != CT_DOUBLE) {
			snprintf(out_err, out_err_len, "val: '%f' requires a DOUBLE column\n", vals_node->double_val);
			return false;
		}

		if (vals_node->is_bool && column->type != CT_TINYINT) {
			snprintf(out_err, out_err_len, "val: '%d' requires a TINYINT column\n", vals_node->bool_val);
			return false;
		}
	} else if (node->node_type == AST_TYPE_INS_EXPROP) {
		// math expression have to have a numeric column data type for obvious reasons
		if (column->type != CT_INTEGER && column->type != CT_DOUBLE) {
			snprintf(out_err, out_err_len, "math expressions requires either a INTEGER or DOUBLE column\n");
			return false;
		}
	} else {
		BUG_ON_CUSTOM_MSG(true, "handler not implemented yet\n");
	}

	return true;
}

static bool validate_values(struct database *db, struct ast_ins_insvals_node *insvals_node, char *out_err, size_t out_err_len)
{
	struct table *table;
	struct list_head *pos1, *pos2;
	struct ast_node *tmp_entry1, *tmp_entry2;
	struct ast_ins_inscols_node *inscols_node;
	struct ast_ins_column_node *col_node;
	struct ast_ins_values_node *vals_node;
	/* columns can be specified in an order that's different from the order defined in the CREATE stmt */
	int column_order[TABLE_MAX_COLUMNS];
	int opt_col_list_idx;
	int vals_idx;

	table = database_table_get(db, insvals_node->table_name);

	if (!insvals_node->opt_column_list) {
		/* column list wasn't specified, so it follows the natural order defined in the CREATE stmt */
		for (int i = 0; i < table->column_count; i++)
			column_order[i] = i;
	} else {
		/* column list was defined, so we create a mapping of that order */

		// find AST_TYPE_INS_INSCOLS node
		list_for_each(pos1, insvals_node->node_children_head)
		{
			tmp_entry1 = list_entry(pos1, typeof(*tmp_entry1), head);
			if (tmp_entry1->node_type == AST_TYPE_INS_INSCOLS) {
				inscols_node = (struct ast_ins_inscols_node*)tmp_entry1;
				break;
			}
		}

		opt_col_list_idx = 0;
		list_for_each(pos2, inscols_node->node_children_head)
		{
			col_node = list_entry(pos2, typeof(*col_node), head);

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

	/* check if values match column types */
	list_for_each(pos1, insvals_node->node_children_head)
	{
		tmp_entry1 = list_entry(pos1, typeof(*tmp_entry1), head);
		if (tmp_entry1->node_type == AST_TYPE_INS_VALUES) {
			vals_node = (struct ast_ins_values_node*)tmp_entry1;

			vals_idx = 0;
			list_for_each(pos2, vals_node->node_children_head)
			{
				tmp_entry2 = list_entry(pos2, typeof(*tmp_entry2), head);

				struct column *column = &table->columns[column_order[vals_idx]];

				if (!check_value_for_column(column, tmp_entry2, out_err, out_err_len)) {
					return false;
				}
				vals_idx++;
			}
		}
	}

	return true;
}

bool semantic_analyse_insert_stmt(struct database *db, struct ast_node *node, char *out_err, size_t out_err_len)
{
	struct ast_ins_insvals_node *insvals_node;

	insvals_node = (struct ast_ins_insvals_node*)node;

	/* validate table */
	if (!validate_table(db, insvals_node, out_err, out_err_len))
		return false;

	/* check whether all rows have the same number of terms */
	if (!validate_number_terms(insvals_node, out_err, out_err_len))
		return false;

	/* when column list is specified, we can validate a few more things */
	if (insvals_node->opt_column_list && !validate_column_list(db, insvals_node, out_err, out_err_len))
		return false;

	if (!validate_values(db, insvals_node, out_err, out_err_len))
		return false;

	/**
	 * TODO list:
	 * 	-> validate if all not null columns have values
	 * 	-> check if unique constraints are not violated (probably later when index management is in place)
	 *
	 */

	return true;
}
