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

struct alias_value {
	char table_name[TABLE_MAX_NAME + 1 /* NUL char */];
	char col_name[TABLE_MAX_COLUMN_NAME + 1 /* NUL char */]; // optional
};

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

static bool check_table_alias(struct ast_node *root, char *out_err, size_t out_err_len, struct hashtable *out_ht)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_alias_node *alias_node;
	struct ast_sel_table_node *table_node;
	struct alias_value alias_value;

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

				memzero(&alias_value, sizeof(alias_value));
				memccpy(alias_value.table_name, value, 0, sizeof(alias_value.table_name));

				if (!hashtable_put(out_ht, key, strlen(key) + 1, &alias_value, sizeof(alias_value))) {
					snprintf(out_err, out_err_len, "semantic phase: internal error\n");
					return false;
				}
			}
		}
	} else {
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_table_alias(tmp_entry, out_err, out_err_len, out_ht))
				return false;
		}
	}

	return true;
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

static bool check_column_alias(struct database *db, struct ast_node *root, char *out_err, size_t out_err_len,
		struct hashtable *table_alias, struct hashtable *out_col_alias)
{

	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_alias_node *alias_node;
	struct ast_sel_exprval_node *val_node;
	struct ast_sel_fieldname_node *field_node;
	struct hashtable_value *table_lookup;
	struct alias_value value;

	if (root->node_type == AST_TYPE_SEL_ALIAS) {
		alias_node = (typeof(alias_node))root;

		char *key = alias_node->alias_value;

		list_for_each(pos, alias_node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (tmp_entry->node_type == AST_TYPE_SEL_EXPRVAL) {
				val_node = (typeof(val_node))tmp_entry;

				if (val_node->value_type.is_name) {
					memzero(&value, sizeof(value));
					memccpy(value.col_name, val_node->name_val, 0, sizeof(value.col_name));
				}
			} else if (tmp_entry->node_type == AST_TYPE_SEL_FIELDNAME) {
				field_node = (typeof(field_node))tmp_entry;

				memzero(&value, sizeof(value));
				memccpy(value.col_name, field_node->col_name, 0, sizeof(value.col_name));

				/* check of table existence or table alias */
				table_lookup = hashtable_get(table_alias, field_node->table_name,
								strlen(field_node->table_name) + 1);

				if (table_lookup) {
					/* found table alias. store the original table name to make other checks easier */
					memccpy(value.table_name,
						((struct alias_value*)table_lookup->content)->table_name,
						0,
						sizeof(value.table_name));
				} else if (database_table_exists(db, value.table_name)) {
					/* query contains full qualified name <table>.<column>. keep it as it is */
					memccpy(value.table_name, field_node->table_name, 0, sizeof(value.table_name));
				} else {
					snprintf(out_err, out_err_len, "alias/table name '%.128s' is invalid\n", key);
					return false;
				}

			} else {
				/* table nodes are not used in this validation, so just move on */
				continue;
			}

			/* is the name valid? column aliases follow table nomenclature rules */
			if (!table_validate_column_name(key)) {
				snprintf(out_err, out_err_len, "alias name '%.128s' is invalid\n", key);
				return false;
			}

			/* was this alias already declared ? (column) */
			if (hashtable_get(out_col_alias, key, strlen(key) + 1)) {
				// fun fact, SQLITE doesn't validate if aliases are unique
				snprintf(out_err, out_err_len, "Not unique column/alias: '%.128s'\n", key);
				return false;
			}

			/* does it conflict with any table aliases ? */
			if (hashtable_get(table_alias, key, strlen(key) + 1)) {
				// fun fact, SQLITE doesn't validate if aliases are unique
				snprintf(out_err, out_err_len, "Not unique column/alias: '%.128s'\n", key);
				return false;
			}

			if (!hashtable_put(out_col_alias, key, strlen(key) + 1, &value, sizeof(value))) {
				snprintf(out_err, out_err_len, "semantic phase: internal error\n");
				return false;
			}
		}
	} else {
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_column_alias(db, tmp_entry, out_err, out_err_len, table_alias, out_col_alias))
				return false;
		}
	}

	return true;
}

bool semantic_analyse_select_stmt(struct database *db, struct ast_node *node, char *out_err, size_t out_err_len)
{
	struct hashtable table_alias = {0};
	struct hashtable column_alias = {0};

	if (!hashtable_init(&table_alias, &hashtable_str_compare, &hashtable_str_hash)) {
		snprintf(out_err, out_err_len, "semantic phase: internal error\n");
		goto err;
	}

	if (!hashtable_init(&column_alias, &hashtable_str_compare, &hashtable_str_hash)) {
		snprintf(out_err, out_err_len, "semantic phase: internal error\n");
		goto err;
	}

	/* does tables exist? */
	if (!check_table_names(db, node, out_err, out_err_len))
		goto err_table_names;

	/* build table alias hashtable; check for table alias duplicates */
	if (!check_table_alias(node, out_err, out_err_len, &table_alias))
		goto err_table_alias;

	/* build column alias hashtable; check for column alias validity */
	if (!check_column_alias(db, node, out_err, out_err_len, &table_alias, &column_alias))
		goto err_column_alias;

	/*
	 * TODO:
	 * - Check for duplicate / invalid aliases (table done, columns done)
	 * 	-> Need to change implementation though... instead of storing as KV alias->table_name (for table aliases)
	 * 	   and alias->?? (for column), I need to find a common denominator. Otherwise, it will become hard to
	 * 	   validate the existence of columns at some point... my best guess, it to use some structure that resembles
	 * 	   the ast_sel_fieldname_nome...  -> I think I've done it (TBC)
	 * - check field name (full qualified ones) as they can contain either table name or alias
	 * - check for columns existence
	 * - check for ambiguous columns on join statements
	 */

	/* cleanup */
	hashtable_foreach(&column_alias, &free_hashmap_entries, NULL);
	hashtable_free(&column_alias);

	hashtable_foreach(&table_alias, &free_hashmap_entries, NULL);
	hashtable_free(&table_alias);

	return true;

err_column_alias:
err_table_names:
err_table_alias:
	hashtable_foreach(&column_alias, &free_hashmap_entries, NULL);
	hashtable_free(&column_alias);

	hashtable_foreach(&table_alias, &free_hashmap_entries, NULL);
	hashtable_free(&table_alias);
err:
	return false;
}
