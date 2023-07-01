/*
 * semantic_insert.c
 *
 *  Created on: 1/07/2023
 *      Author: paulo
 */

#include <parser/semantic.h>
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

	if (!database_table_exists(db, insvals_node->table_name))
		return false;

	table = database_table_get(db, insvals_node->table_name);

	for (int i = 0; i < table->column_count; i++) {
		if (strncmp(table->columns[i].name, col_node->name, sizeof(table->columns[i].name)) == 0) {
			return true;
		}
	}

	return false;
}

static bool validate_columns(struct database *db, struct ast_ins_insvals_node *insvals_node, char *out_err, size_t out_err_len)
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

bool validate_number_terms(struct ast_ins_insvals_node *insvals_node, char *out_err, size_t out_err_len)
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

bool semantic_analyse_insert_stmt(struct database *db, struct ast_node *node, char *out_err, size_t out_err_len)
{
	struct ast_ins_insvals_node *insvals_node;

	insvals_node = (struct ast_ins_insvals_node*)node;

	/* validate table */
	if (!validate_table(db, insvals_node, out_err, out_err_len))
		return false;

	/* when column list is specified, we can validate a few more things */
	if (insvals_node->opt_column_list && !validate_columns(db, insvals_node, out_err, out_err_len))
		return false;

	/* check whether all rows have the same number of terms */
	if (!validate_number_terms(insvals_node, out_err, out_err_len))
		return false;

	return true;
}
