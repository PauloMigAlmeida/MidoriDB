/*
 * semantic.c
 *
 *  Created on: 7/06/2023
 *      Author: paulo
 */

#include <parser/semantic.h>
#include <datastructure/hashtable.h>
#include <datastructure/linkedlist.h>
#include <primitive/table.h>

static int free_str_entries(struct hashtable *hashtable, const void *key, size_t klen,
		const void *value, size_t vlen, void *arg)
{
	struct hashtable_entry *entry = NULL;
	UNUSED(arg);
	UNUSED(value);
	UNUSED(vlen);

	entry = hashtable_remove(hashtable, key, klen);
	free(entry->key.content);
	free(entry->value.content);
	free(entry);

	return 0;
}

static bool validate_create_stmt(struct ast_node *node, char *out_err, size_t out_err_len)
{
	struct hashtable ht = {0};
	struct ast_create_node *create_node;
	struct ast_column_def_node *coldef_node;
	struct ast_index_def_node *idxdef_node;
	struct ast_index_column_node *idxcol_node;
	struct list_head *pos1 = NULL;
	struct list_head *pos2 = NULL;
	struct ast_node *tmp_entry;

	if (!hashtable_init(&ht, &hashtable_str_compare, &hashtable_str_hash)) {
		snprintf(out_err, out_err_len, "semantic phase: internal error\n");
		goto err;
	}

	/* validate table name */
	create_node = (struct ast_create_node*)node;
	if (!table_validate_name(create_node->table_name)) {
		snprintf(out_err, out_err_len,
				"semantic phase: table name '%s' is invalid\n",
				create_node->table_name);

		goto err_table_name;
	}

	/* validate columns */
	list_for_each(pos1, create_node->node_children_head)
	{
		tmp_entry = list_entry(pos1, typeof(*tmp_entry), head);
		if (tmp_entry->node_type == AST_TYPE_COLUMNDEF) {
			coldef_node = (struct ast_column_def_node*)tmp_entry;

			if (hashtable_get(&ht, coldef_node->name, strlen(coldef_node->name) + 1)) {
				snprintf(out_err, out_err_len,
						"semantic phase: duplicate column name: '%s'\n",
						coldef_node->name);

				goto err_dup_col_name;
			}

			if (!table_validate_column_name(coldef_node->name)) {
				snprintf(out_err, out_err_len,
						"semantic phase: column name '%s' is invalid\n",
						coldef_node->name);

				goto err_column_name;
			}

			/*TODO maybe I should create a Set datastructure -
			 * that would make this code easier on the eyes*/
			if (!hashtable_put(&ht, coldef_node->name, strlen(coldef_node->name) + 1,
						coldef_node->name,
						strlen(coldef_node->name) + 1)) {

				snprintf(out_err, out_err_len, "semantic phase: internal error\n");

				goto err_ht_put_col;
			}
		}
	}

	/*
	 * validate indexes / primary keys
	 *
	 * they can't be done at the same time as the column validation given that
	 * there is no strict other in which primary keys and indexes (after column definition)
	 * could be defined as per the parser rules.... so I need to make sure that I gathered
	 * all columns first :-)
	 */
	list_for_each(pos1, create_node->node_children_head)
	{
		tmp_entry = list_entry(pos1, typeof(*tmp_entry), head);
		if (tmp_entry->node_type == AST_TYPE_INDEXDEF) {
			idxdef_node = (struct ast_index_def_node*)tmp_entry;

			list_for_each(pos2, idxdef_node->node_children_head)
			{
				idxcol_node = list_entry(pos2, typeof(*idxcol_node), head);

				if (!hashtable_get(&ht, idxcol_node->name, strlen(idxcol_node->name) + 1)) {

					snprintf(out_err, out_err_len,
							"semantic phase: invalid column: '%s'\n",
							idxcol_node->name);

					goto err_idx_pk_col;
				}
			}
		}
	}

	/* clean up */
	hashtable_foreach(&ht, &free_str_entries, NULL);
	hashtable_free(&ht);

	return true;

	err_idx_pk_col:
	err_ht_put_col:
	err_column_name:
	err_dup_col_name:
	hashtable_foreach(&ht, &free_str_entries, NULL);
	err_table_name:
	hashtable_free(&ht);
	err:
	return false;
}

bool semantic_analyse(struct ast_node *node, char *out_err, size_t out_err_len)
{
	/* sanity checks */
	BUG_ON(!node || !out_err || out_err_len == 0);

	if (node->node_type == AST_TYPE_CREATE)
		return validate_create_stmt(node, out_err, out_err_len);
	else
		/* semantic analysis not implemented for that yet */
		BUG_ON(true);

	return false;
}
