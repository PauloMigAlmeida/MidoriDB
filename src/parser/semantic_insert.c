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

bool semantic_analyse_insert_stmt(struct database *db, struct ast_node *node, char *out_err, size_t out_err_len)
{
	struct hashtable ht = {0};
	struct ast_ins_insvals_node *insvals_node;
	struct ast_ins_inscols_node *inscols_node;
	struct ast_ins_values_node *vals_node;
	struct ast_ins_column_node *col_node;
	struct list_head *pos1, *pos2;
	struct ast_node *tmp_entry;
	int num_terms = -1;

	if (!hashtable_init(&ht, &hashtable_str_compare, &hashtable_str_hash)) {
		snprintf(out_err, out_err_len, "semantic phase: internal error\n");
		goto err;
	}

	/* validate table name */
	insvals_node = (struct ast_ins_insvals_node*)node;
	if (!table_validate_name(insvals_node->table_name)) {
		snprintf(out_err, out_err_len, "table name '%s' is invalid\n", insvals_node->table_name);

		goto err_table_name;
	}

	/* when column list is specified, we can validate a few more things */
	if (insvals_node->opt_column_list) {

		/* validate column names */
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

					if (!table_validate_column_name(col_node->name)) {
						snprintf(out_err, out_err_len, "column name '%s' is invalid\n",
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

	}

	/* check whether all rows have the same number of terms */
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
err_table_name:
	hashtable_free(&ht);
err:
	return false;
}
