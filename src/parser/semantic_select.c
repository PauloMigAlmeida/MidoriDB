/*
 * semantic_select.c
 *
 *  Created on: 10/10/2023
 *      Author: paulo
 */

#include <parser/semantic.h>
#include <primitive/table.h>
#include <primitive/column.h>
#include <datastructure/hashtable.h>
#include <datastructure/linkedlist.h>

static void free_hashmap_entries(struct hashtable *hashtable, const void *key, size_t klen,
		const void *value, size_t vlen, void *arg)
{
	struct hashtable_entry *entry = NULL;
	UNUSED(arg);
	UNUSED(value);
	UNUSED(vlen);

	entry = hashtable_remove(hashtable, key, klen);
	hashtable_free_entry(entry);
}

static bool do_build_alias_map(struct ast_node *root, char *out_err, size_t out_err_len, struct hashtable *out_ht)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_alias_node *alias_node;
	struct ast_sel_table_node *table_node;

	if (root->node_type == AST_TYPE_SEL_ALIAS) {
		alias_node = (typeof(alias_node))root;

		list_for_each(pos, alias_node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (tmp_entry->node_type == AST_TYPE_SEL_TABLE) {
				table_node = (typeof(table_node))tmp_entry;
				char *key = alias_node->alias_value;
				char *value = table_node->table_name;

				/* is the name valid? table aliases follow table nomenclature rules */
				if (!table_validate_name(key)) {
					snprintf(out_err, out_err_len, "alias name '%.128s' is invalid\n", key);
					return false;
				}

				/* was this alias already declared ? */
				if (hashtable_get(out_ht, key, strlen(key) + 1)) {
					// fun fact, SQLITE doesn't validate if aliases are unique
					snprintf(out_err, out_err_len, "Not unique table/alias: '%.128s'\n", key);
					return false;
				}

				if (!hashtable_put(out_ht, key, strlen(key) + 1, value, strlen(value) + 1)) {
					snprintf(out_err, out_err_len, "semantic phase: internal error\n");
					return false;
				}
			}
		}
	} else {
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!do_build_alias_map(tmp_entry, out_err, out_err_len, out_ht))
				return false;
		}
	}

	return true;
}

static bool build_table_alias_map(struct ast_node *root, char *out_err, size_t out_err_len, struct hashtable *out_ht)
{

	if (!hashtable_init(out_ht, &hashtable_str_compare, &hashtable_str_hash))
		goto err;

	if (!do_build_alias_map(root, out_err, out_err_len, out_ht))
		return false;

	return true;

err:
	snprintf(out_err, out_err_len, "semantic phase: internal error\n");
	return false;
}

static bool check_table_names(struct database *db, struct ast_node *root, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_table_node *table_node;

	if (root->node_type == AST_TYPE_SEL_TABLE) {

		table_node = (typeof(table_node))root;

		if (!table_validate_name(table_node->table_name)) {
			snprintf(out_err, out_err_len, "table name '%s' is invalid\n", table_node->table_name);
			return false;
		}

		if (!database_table_exists(db, table_node->table_name)) {
			snprintf(out_err, out_err_len, "table '%s' doesn't exist\n", table_node->table_name);
			return false;
		}

	}

	list_for_each(pos, root->node_children_head)
	{
		tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

		if (!check_table_names(db, tmp_entry, out_err, out_err_len))
			return false;
	}

	return true;
}

bool semantic_analyse_select_stmt(struct database *db, struct ast_node *node, char *out_err, size_t out_err_len)
{
	bool ret = true;
	struct hashtable ht = {0};

	/* build table alias hashtable; check for duplicates */
	if (!(ret = build_table_alias_map(node, out_err, out_err_len, &ht)))
		goto cleanup;

	/* does tables exist? */
	if (!(ret = check_table_names(db, node, out_err, out_err_len)))
		goto cleanup;

	/*
	 * TODO:
	 * - Check for duplicate / invalid aliases (table done, columns tbi)
	 * - check field name (full qualified ones) as they can contain either table name or alias
	 * - check for columns existence
	 * - check for ambiguous columns on join statements
	 */

cleanup:
	hashtable_foreach(&ht, &free_hashmap_entries, NULL);
	hashtable_free(&ht);

	return ret;
}
