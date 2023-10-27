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
	char col_name[TABLE_MAX_COLUMN_NAME + 1 /* NUL char */];
	struct {
		bool is_table; /* Ex.: select x.* from A as x */
		bool is_field; /* Ex.: select f1 as val from A*/
		bool is_fqfield; /* Ex.: select x.f1 as val from A as x */
		bool is_expre; /* Ex.: select f1 * 2 as val from A */
		bool is_count; /* Ex.: select count(f1) as val from A */
	} type;
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

static struct ast_node* find_node(struct ast_node *root, enum ast_node_type node_type)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_node *ret = NULL;

	if (root->node_type == node_type) {
		return root;
	} else {
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			ret = find_node(tmp_entry, node_type);
			if (ret)
				return ret;
		}
	}

	return ret;
}

static inline bool in_select_clause(struct ast_node *node)
{
	return node->node_type != AST_TYPE_SEL_WHERE
			&& node->node_type != AST_TYPE_SEL_JOIN
			&& node->node_type != AST_TYPE_SEL_HAVING
			&& node->node_type != AST_TYPE_SEL_GROUPBY
			&& node->node_type != AST_TYPE_SEL_ORDERBYLIST
			&& node->node_type != AST_TYPE_SEL_LIMIT;
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
				alias_value.type.is_table = true;
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

				memzero(&value, sizeof(value));
				if (val_node->value_type.is_name) {
					value.type.is_field = true;
					memccpy(value.col_name, val_node->name_val, 0, sizeof(value.col_name));
				} else {
					/* raw value such as SELECT 1 as bla */
					value.type.is_expre = true;
				}
			} else if (tmp_entry->node_type == AST_TYPE_SEL_EXPROP) {
				/* recursive expression such as SELECT f1 * 2 as val FROM a; */
				memzero(&value, sizeof(value));
				value.type.is_expre = true;
			} else if (tmp_entry->node_type == AST_TYPE_SEL_COUNT) {
				/* COUNT function such as SELECT COUNT(f1) as val FROM a; */
				memzero(&value, sizeof(value));
				value.type.is_count = true;
			} else if (tmp_entry->node_type == AST_TYPE_SEL_FIELDNAME) {
				field_node = (typeof(field_node))tmp_entry;

				memzero(&value, sizeof(value));
				value.type.is_fqfield = true;
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

			} else if (tmp_entry->node_type == AST_TYPE_SEL_TABLE) {
				/* table nodes are not used in this validation, so just move on */
				continue;
			} else {
				/* is there an edge case that I haven't foreseen */
				BUG_GENERIC();
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

static int tables_with_column_name(struct database *db, struct ast_node *root, char *col_name)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_table_node *table_node;
	struct table *table;
	int count = 0;

	if (root->node_type == AST_TYPE_SEL_TABLE) {
		table_node = (typeof(table_node))root;
		table = database_table_get(db, table_node->table_name);

		for (int i = 0; i < table->column_count; i++) {
			if (strcmp(table->columns[i].name, col_name) == 0) {
				count++;
				break;
			}
		}
	} else {
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);
			count += tables_with_column_name(db, tmp_entry, col_name);
		}
	}

	return count;
}

static bool check_fqfield_table(struct database *db, struct ast_node *root, struct ast_sel_fieldname_node *field_node)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_table_node *table_node;
	struct table *table;

	if (root->node_type == AST_TYPE_SEL_TABLE) {
		table_node = (typeof(table_node))root;

		if (strcmp(table_node->table_name, field_node->table_name) == 0) {
			table = database_table_get(db, table_node->table_name);

			for (int i = 0; i < table->column_count; i++) {
				if (strcmp(table->columns[i].name, field_node->col_name) == 0) {
					return true;
				}
			}
		}

	} else {
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);
			if (check_fqfield_table(db, tmp_entry, field_node))
				return true;
		}
	}

	return false;
}

static bool check_column_names_select(struct database *db, struct ast_node *root, struct ast_node *node, char *out_err,
		size_t out_err_len, struct hashtable *table_alias, struct hashtable *column_alias)
{

	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_exprval_node *val_node;
	struct ast_sel_fieldname_node *field_node;
	struct hashtable_value *ht_value;
	struct alias_value *alias_value;
	struct table *table;
	int count = 0;

	/* base case 1 */
	if (!in_select_clause(node)) {
		return true;
	}
	/* base case 2 */
	else if (node->node_type == AST_TYPE_SEL_EXPRVAL) {
		val_node = (typeof(val_node))node;

		if (val_node->value_type.is_name) {
			count = tables_with_column_name(db, root, val_node->name_val);

			if (count == 0) {
				snprintf(out_err, out_err_len, "no such column: '%.128s'\n",
						val_node->name_val);
				return false;
			} else if (count > 1) {
				snprintf(out_err, out_err_len, "ambiguous column name: '%.128s'\n",
						val_node->name_val);
				return false;
			}
		}
	}
	/* base case 3 */
	else if (node->node_type == AST_TYPE_SEL_FIELDNAME) {
		field_node = (typeof(field_node))node;

		ht_value = hashtable_get(table_alias, field_node->table_name, strlen(field_node->table_name) + 1);

		if (ht_value) {
			/*
			 * field name using a table alias such as:
			 * 	- SELECT x.f1 from A as x;
			 */
			alias_value = (typeof(alias_value))ht_value->content;
			table = database_table_get(db, alias_value->table_name);
		} else {
			/*
			 * full qualified name without alias such as:
			 * 	-  SELECT A.f1 from A;
			 * Not very smart but still valid SQL so we got to validate it
			 */
			if (!database_table_exists(db, field_node->table_name)) {
				snprintf(out_err, out_err_len, "table doesn't exist: '%.128s'\n",
						field_node->table_name);
				return false;
			}

			/**
			 * check if full qualified name points to a table that is not part of the
			 * FROM clause.
			 * 	- SELECT B.f1 from A;
			 */
			if (!check_fqfield_table(db, root, field_node)) {
				snprintf(out_err, out_err_len,
						"table is not part of from clause: '%.128s'\n",
						field_node->table_name);
				return false;
			}

			table = database_table_get(db, field_node->table_name);
		}

		for (int i = 0; i < table->column_count; i++) {
			if (strcmp(table->columns[i].name, field_node->col_name) == 0) {
				count = 1;
				break;
			}
		}

		if (count == 0) {
			snprintf(out_err, out_err_len, "no such column: '%.128s'.'%.128s'\n", field_node->table_name,
					field_node->col_name);
			return false;
		}

	}
	/* recursion  */
	else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_column_names_select(db, root, tmp_entry, out_err, out_err_len, table_alias,
							column_alias))
				return false;
		}
	}

	return true;
}

static bool check_column_names_where(struct database *db, struct ast_node *root, struct ast_node *node,
		char *out_err, size_t out_err_len, struct hashtable *table_alias, struct hashtable *column_alias)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_exprval_node *val_node;
	struct ast_sel_fieldname_node *field_node;
	struct hashtable_value *ht_value;
	struct table *table;
	struct alias_value *alias_value;
	int count = 0;

	if (node->node_type == AST_TYPE_SEL_EXPRVAL) {
		val_node = (typeof(val_node))node;

		if (val_node->value_type.is_name) {

			/* is it a column alias? */
			if (hashtable_get(column_alias, val_node->name_val, strlen(val_node->name_val) + 1))
				return true; /* we are good */

			/* is it an actual column? */
			count = tables_with_column_name(db, root, val_node->name_val);

			if (count == 0) {
				snprintf(out_err, out_err_len, "no such column: '%.128s'\n",
						val_node->name_val);
				return false;
			} else if (count > 1) {
				snprintf(out_err, out_err_len, "ambiguous column name: '%.128s'\n",
						val_node->name_val);
				return false;
			}
		}

	} else if (node->node_type == AST_TYPE_SEL_FIELDNAME) {
		field_node = (typeof(field_node))node;

		ht_value = hashtable_get(table_alias, field_node->table_name, strlen(field_node->table_name) + 1);

		if (ht_value) {
			/*
			 * field name using a table alias such as:
			 * 	- SELECT x.f1 from A as x;
			 */
			alias_value = (typeof(alias_value))ht_value->content;
			table = database_table_get(db, alias_value->table_name);
		} else {
			/*
			 * full qualified name without alias such as:
			 * 	-  SELECT A.f1 from A;
			 * Not very smart but still valid SQL so we got to validate it
			 */
			if (!database_table_exists(db, field_node->table_name)) {
				snprintf(out_err, out_err_len, "table doesn't exist: '%.128s'\n",
						field_node->table_name);
				return false;
			}

			/* full qualified name that points to a table that is not part of the FROM clause */
			if (!check_fqfield_table(db, root, field_node)) {
				snprintf(out_err, out_err_len,
						"table is not part of from clause: '%.128s'\n",
						field_node->table_name);
				return false;
			}

			table = database_table_get(db, field_node->table_name);
		}

		for (int i = 0; i < table->column_count; i++) {
			if (strcmp(table->columns[i].name, field_node->col_name) == 0) {
				count = 1;
				break;
			}
		}

		if (count == 0) {
			snprintf(out_err, out_err_len, "no such column: '%.128s'.'%.128s'\n", field_node->table_name,
					field_node->col_name);
			return false;
		}

	} else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_column_names_where(db, root, tmp_entry, out_err, out_err_len, table_alias,
							column_alias))
				return false;
		}
	}

	return true;
}

static bool check_column_names_groupby(struct database *db, struct ast_node *root, struct ast_node *node, char *out_err,
		size_t out_err_len, struct hashtable *table_alias, struct hashtable *column_alias)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_exprval_node *val_node;
	struct ast_sel_fieldname_node *field_node;
	struct hashtable_value *ht_value;
	struct table *table;
	struct alias_value *alias_value;
	int count = 0;

	if (node->node_type == AST_TYPE_SEL_EXPRVAL) {
		val_node = (typeof(val_node))node;

		if (val_node->value_type.is_name) {

			/* is it a column alias? */
			if (hashtable_get(column_alias, val_node->name_val, strlen(val_node->name_val) + 1))
				return true; /* we are good */

			/* is it an actual column? */
			count = tables_with_column_name(db, root, val_node->name_val);

			if (count == 0) {
				snprintf(out_err, out_err_len, "no such column: '%.128s'\n",
						val_node->name_val);
				return false;
			} else if (count > 1) {
				snprintf(out_err, out_err_len, "ambiguous column name: '%.128s'\n",
						val_node->name_val);
				return false;
			}
		}

	} else if (node->node_type == AST_TYPE_SEL_FIELDNAME) {
		field_node = (typeof(field_node))node;

		ht_value = hashtable_get(table_alias, field_node->table_name, strlen(field_node->table_name) + 1);

		if (ht_value) {
			/*
			 * field name using a table alias such as:
			 * 	- SELECT x.f1 from A as x;
			 */
			alias_value = (typeof(alias_value))ht_value->content;
			table = database_table_get(db, alias_value->table_name);
		} else {
			/*
			 * full qualified name without alias such as:
			 * 	-  SELECT A.f1 from A;
			 * Not very smart but still valid SQL so we got to validate it
			 */
			if (!database_table_exists(db, field_node->table_name)) {
				snprintf(out_err, out_err_len, "table doesn't exist: '%.128s'\n",
						field_node->table_name);
				return false;
			}

			/* full qualified name that points to a table that is not part of the FROM clause */
			if (!check_fqfield_table(db, root, field_node)) {
				snprintf(out_err, out_err_len,
						"table is not part of from clause: '%.128s'\n",
						field_node->table_name);
				return false;
			}

			table = database_table_get(db, field_node->table_name);
		}

		for (int i = 0; i < table->column_count; i++) {
			if (strcmp(table->columns[i].name, field_node->col_name) == 0) {
				count = 1;
				break;
			}
		}

		if (count == 0) {
			snprintf(out_err, out_err_len, "no such column: '%.128s'.'%.128s'\n", field_node->table_name,
					field_node->col_name);
			return false;
		}

	} else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_column_names_where(db, root, tmp_entry, out_err, out_err_len, table_alias,
							column_alias))
				return false;
		}
	}

	return true;
}

static bool check_column_names_orderby(struct database *db, struct ast_node *root, struct ast_node *node, char *out_err,
		size_t out_err_len, struct hashtable *table_alias, struct hashtable *column_alias)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_exprval_node *valn;
	struct ast_sel_fieldname_node *field_node;
	struct hashtable_value *ht_value;
	struct table *table;
	struct alias_value *alias_value;
	int count = 0;

	if (node->node_type == AST_TYPE_SEL_ORDERBYITEM) {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (tmp_entry->node_type == AST_TYPE_SEL_EXPRVAL) {
				valn = (typeof(valn))tmp_entry;

				if (valn->value_type.is_name) {

					/* is it a column alias? */
					if (hashtable_get(column_alias, valn->name_val, strlen(valn->name_val) + 1))
						continue; /* we are good */

					/* is it an actual column? */
					count = tables_with_column_name(db, root, valn->name_val);

					if (count == 0) {
						snprintf(out_err, out_err_len, "no such column: '%.128s'\n",
								valn->name_val);
						return false;
					} else if (count > 1) {
						snprintf(out_err, out_err_len, "ambiguous column name: '%.128s'\n",
								valn->name_val);
						return false;
					}
				}
			} else if (tmp_entry->node_type == AST_TYPE_SEL_FIELDNAME) {
				field_node = (typeof(field_node))tmp_entry;

				ht_value = hashtable_get(table_alias, field_node->table_name,
								strlen(field_node->table_name) + 1);

				if (ht_value) {
					/*
					 * field name using a table alias such as:
					 * 	- SELECT x.f1 from A as x;
					 */
					alias_value = (typeof(alias_value))ht_value->content;
					table = database_table_get(db, alias_value->table_name);
				} else {
					/*
					 * full qualified name without alias such as:
					 * 	-  SELECT A.f1 from A;
					 * Not very smart but still valid SQL so we got to validate it
					 */
					if (!database_table_exists(db, field_node->table_name)) {
						snprintf(out_err, out_err_len, "table doesn't exist: '%.128s'\n",
								field_node->table_name);
						return false;
					}

					/* full qualified name that points to a table that is not part of the FROM clause */
					if (!check_fqfield_table(db, root, field_node)) {
						snprintf(out_err, out_err_len,
								"table is not part of from clause: '%.128s'\n",
								field_node->table_name);
						return false;
					}

					table = database_table_get(db, field_node->table_name);
				}

				for (int i = 0; i < table->column_count; i++) {
					if (strcmp(table->columns[i].name, field_node->col_name) == 0) {
						count = 1;
						break;
					}
				}

				if (count == 0) {
					snprintf(out_err, out_err_len, "no such column: '%.128s'.'%.128s'\n",
							field_node->table_name,
							field_node->col_name);
					return false;
				}

			}
		}

	} else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_column_names_orderby(db, root, tmp_entry, out_err, out_err_len, table_alias,
							column_alias))
				return false;
		}
	}

	return true;
}

static bool check_column_names_having(struct database *db, struct ast_node *root, struct ast_node *node, char *out_err,
		size_t out_err_len, struct hashtable *table_alias, struct hashtable *column_alias)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_exprval_node *val_node;
	struct ast_sel_fieldname_node *field_node;
	struct hashtable_value *ht_value;
	struct table *table;
	struct alias_value *alias_value;
	int count = 0;

	if (node->node_type == AST_TYPE_SEL_EXPRVAL) {
		val_node = (typeof(val_node))node;

		if (val_node->value_type.is_name) {

			/* is it a column alias? */
			if (hashtable_get(column_alias, val_node->name_val, strlen(val_node->name_val) + 1))
				return true; /* we are good */

			/* is it an actual column? */
			count = tables_with_column_name(db, root, val_node->name_val);

			if (count == 0) {
				snprintf(out_err, out_err_len, "no such column: '%.128s'\n",
						val_node->name_val);
				return false;
			} else if (count > 1) {
				snprintf(out_err, out_err_len, "ambiguous column name: '%.128s'\n",
						val_node->name_val);
				return false;
			}
		}

	} else if (node->node_type == AST_TYPE_SEL_FIELDNAME) {
		field_node = (typeof(field_node))node;

		ht_value = hashtable_get(table_alias, field_node->table_name, strlen(field_node->table_name) + 1);

		if (ht_value) {
			/*
			 * field name using a table alias such as:
			 * 	- SELECT x.f1 from A as x;
			 */
			alias_value = (typeof(alias_value))ht_value->content;
			table = database_table_get(db, alias_value->table_name);
		} else {
			/*
			 * full qualified name without alias such as:
			 * 	-  SELECT A.f1 from A;
			 * Not very smart but still valid SQL so we got to validate it
			 */
			if (!database_table_exists(db, field_node->table_name)) {
				snprintf(out_err, out_err_len, "table doesn't exist: '%.128s'\n",
						field_node->table_name);
				return false;
			}

			/* full qualified name that points to a table that is not part of the FROM clause */
			if (!check_fqfield_table(db, root, field_node)) {
				snprintf(out_err, out_err_len,
						"table is not part of from clause: '%.128s'\n",
						field_node->table_name);
				return false;
			}

			table = database_table_get(db, field_node->table_name);
		}

		for (int i = 0; i < table->column_count; i++) {
			if (strcmp(table->columns[i].name, field_node->col_name) == 0) {
				count = 1;
				break;
			}
		}

		if (count == 0) {
			snprintf(out_err, out_err_len, "no such column: '%.128s'.'%.128s'\n", field_node->table_name,
					field_node->col_name);
			return false;
		}

	} else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_column_names_where(db, root, tmp_entry, out_err, out_err_len, table_alias,
							column_alias))
				return false;
		}
	}

	return true;
}

static bool check_column_names(struct database *db, struct ast_node *root, struct ast_node *node, char *out_err,
		size_t out_err_len, struct hashtable *table_alias, struct hashtable *column_alias)
{
	struct ast_node *where_node;
	struct ast_node *groupby_node;
	struct ast_node *orderby_node;
	struct ast_node *having_node;

	where_node = find_node(root, AST_TYPE_SEL_WHERE);
	groupby_node = find_node(root, AST_TYPE_SEL_GROUPBY);
	orderby_node = find_node(root, AST_TYPE_SEL_ORDERBYLIST);
	having_node = find_node(root, AST_TYPE_SEL_HAVING);

	/*
	 * check if EXPRVAL (names) / fieldnames point to something that does not exist or that is ambiguous on SELECT fields.
	 * this handles cases like:
	 *
	 * 	CREATE TABLE A (f1 INT);
	 * 	CREATE TABLE B (f1 INT);
	 *
	 * 	SELECT f2 FROM A; (column doesn't exist)
	 * 	SELECT f1 FROM A, B; (column exists on both A and B so we can't tell from which to fetch data from)
	 * 	SELECT f1 as val, val * 2 as x FROM A; (alias can't be used as the source for yet another alias)
	 */
	if (!check_column_names_select(db, root, node, out_err, out_err_len, table_alias, column_alias))
		return false;

	if (where_node) {
		/*
		 * check if EXPRVAL (names) / fieldnames in the where-clause are either columns or alias...
		 * Also ensure that none of them are ambiguous if more than 1 table is used.
		 * This should handle cases like:
		 *
		 * 	CREATE TABLE A (f1 INT);
		 * 	CREATE TABLE B (f1 INT);
		 *
		 * 	SELECT f1 FROM A WHERE f1 = 1; (okay)
		 * 	SELECT f1 as val FROM A WHERE val = 1; (okay)
		 * 	SELECT f1 FROM A, B WHERE f1 = 1; (f1 is ambiguous)
		 * 	SELECT f1 FROM A WHERE f2 = 2; (no such column)
		 */
		if (!check_column_names_where(db, root, where_node, out_err, out_err_len, table_alias, column_alias))
			return false;
	}

	if (groupby_node) {
		/*
		 * check if EXPRVAL (names) / fieldnames in the group-by clause are either columns or alias...
		 * Also ensure that none of them are ambiguous if more than 1 table is used.
		 * This should handle cases like:
		 *
		 * 	CREATE TABLE A (f1 INT);
		 * 	CREATE TABLE B (f1 INT);
		 *
		 * 	SELECT f1 FROM A GROUP BY f1; (okay)
		 * 	SELECT f1 as val FROM A GROUP BY val; (okay)
		 * 	SELECT f1 FROM A, B GROUP BY f1; (f1 is ambiguous)
		 * 	SELECT f1 FROM A GROUP BY f2; (no such column)
		 */
		if (!check_column_names_groupby(db, root, groupby_node, out_err, out_err_len, table_alias,
						column_alias))
			return false;
	}

	if (orderby_node) {
		/*
		 * check if EXPRVAL (names) / fieldnames in the order by clause are either columns or alias... Also ensure that none of them
		 * are ambiguous if more than 1 table is used. This should handle cases like:
		 *
		 * 	CREATE TABLE A (f1 INT);
		 * 	CREATE TABLE B (f1 INT);
		 *
		 * 	SELECT f1 FROM A ORDER BY f1; (okay)
		 * 	SELECT f1 as val FROM A ORDER BY val; (okay)
		 * 	SELECT f1 FROM A, B ORDER BY f1; (f1 is ambiguous)
		 * 	SELECT f1 FROM A ORDER BY f2; (no such column)
		 */
		if (!check_column_names_orderby(db, root, orderby_node, out_err, out_err_len, table_alias,
						column_alias))
			return false;
	}

	if (having_node) {
		/*
		 * check if EXPRVAL (names) / fieldnames in the having-clause are either columns or alias... Also ensure that none of them
		 * are ambiguous if more than 1 table is used. This should handle cases like:
		 *
		 * 	CREATE TABLE A (f1 INT);
		 * 	CREATE TABLE B (f1 INT);
		 *
		 * 	SELECT COUNT(f1) FROM A HAVING COUNT(f1) > 0; (okay)
		 * 	SELECT COUNT(f1) as val FROM A HAVING val > 0; (okay)
		 * 	SELECT COUNT(f1) FROM A, B HAVING COUNT(f1) > 0; (f1 is ambiguous)
		 * 	SELECT COUNT(f1) FROM A ORDER BY COUNT(f2) > 0; (no such column)
		 */
		if (!check_column_names_having(db, root, having_node, out_err, out_err_len, table_alias,
						column_alias))
			return false;
	}

	return true;
}

static bool check_count_function(struct ast_node *node, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_exprval_node *val_node;
	struct ast_sel_fieldname_node *field_node;

	if (node->node_type == AST_TYPE_SEL_COUNT) {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (tmp_entry->node_type == AST_TYPE_SEL_EXPRVAL) {
				val_node = (typeof(val_node))tmp_entry;

				if (!val_node->value_type.is_name) {
					snprintf(out_err, out_err_len, "invalid value for COUNT function\n");
					return false;
				}

			} else if (tmp_entry->node_type == AST_TYPE_SEL_FIELDNAME) {
				field_node = (typeof(field_node))tmp_entry;

			} else {
				snprintf(out_err, out_err_len, "COUNT function can only have '*' or 'fields'\n");
				return false;
			}

		}
	} else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_count_function(tmp_entry, out_err, out_err_len))
				return false;
		}
	}

	return true;
}

static bool check_where_clause_expr(struct ast_node *root, struct ast_node *parent, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;

	if (root->node_type == AST_TYPE_SEL_EXPRVAL || root->node_type == AST_TYPE_SEL_EXPROP) {
		if (parent->node_type == AST_TYPE_SEL_WHERE || parent->node_type == AST_TYPE_SEL_LOGOP) {
			snprintf(out_err, out_err_len,
					"expressions in where clause must be a type of comparison\n");
			return false;
		}
	} else {
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_where_clause_expr(tmp_entry, root, out_err, out_err_len))
				return false;
		}
	}
	return true;
}

static bool check_where_clause_like(struct ast_node *root, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_exprval_node *val_node;
	int i = 0;

	if (root->node_type == AST_TYPE_SEL_LIKE) {
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (i == 0) {

				if (tmp_entry->node_type != AST_TYPE_SEL_FIELDNAME
						&& tmp_entry->node_type != AST_TYPE_SEL_EXPRVAL) {
					snprintf(out_err, out_err_len, "field expected before LIKE function\n");
					return false;
				}

				if (tmp_entry->node_type == AST_TYPE_SEL_EXPRVAL) {
					val_node = (typeof(val_node))tmp_entry;
					if (!val_node->value_type.is_name) {
						snprintf(out_err, out_err_len,
								"field expected before LIKE function\n");
						return false;
					}
				}
			} else {
				val_node = (typeof(val_node))tmp_entry;
				if (val_node->node_type != AST_TYPE_SEL_EXPRVAL || !val_node->value_type.is_str) {
					snprintf(out_err, out_err_len, "raw string expected after LIKE function\n");
					return false;
				}
			}
			i++;

		}
	} else {
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_where_clause_like(tmp_entry, out_err, out_err_len))
				return false;
		}
	}

	return true;
}

static bool check_where_clause_count(struct ast_node *root, char *out_err, size_t out_err_len, struct hashtable *column_alias)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_exprval_node *val_node;
	struct hashtable_value *ht_value;
	struct alias_value *alias_value;

	if (root->node_type == AST_TYPE_SEL_COUNT) {
		snprintf(out_err, out_err_len, "COUNT function can't be used in the where-clause\n");
		return false;
	} else if (root->node_type == AST_TYPE_SEL_EXPRVAL) {
		val_node = (typeof(val_node))root;
		if (val_node->value_type.is_name) {
			ht_value = hashtable_get(column_alias, val_node->name_val, strlen(val_node->name_val) + 1);
			if (ht_value) {
				alias_value = (typeof(alias_value))ht_value->content;
				if (alias_value->type.is_count) {
					snprintf(out_err,
							out_err_len,
							"COUNT function can't be used in the where-clause\n");
					return false;
				}
			}
		}

	} else {
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_where_clause_count(tmp_entry, out_err, out_err_len, column_alias))
				return false;
		}
	}
	return true;
}

static bool check_where_clause(struct ast_node *root, char *out_err, size_t out_err_len, struct hashtable *column_alias)
{
	struct ast_node *where_node;

	where_node = find_node(root, AST_TYPE_SEL_WHERE);

	if (where_node) {
		/*
		 * check if expr do not contain simple exprvals which are syntactically correct but semantically wrong.
		 *
		 * PS.: However. many DB engines like SQLite and Mysql do support it for some reason.
		 *
		 *	SELECT f1 from A where f1 > 2; (okay);
		 *   	SELECT f1 from A where f1; (No CMP);
		 *   	SELECT f1 from A where f1 + 2; (No CMP);
		 *   	SELECT f1 from A where 2 + 2; (No CMP);
		 *   	SELECT f1 from A where 2; (No CMP);
		 *   	SELECT f1 from A where f1 AND 2; (No CMP);
		 */
		if (!check_where_clause_expr(where_node, NULL, out_err, out_err_len))
			return false;

		/*
		 * check if LIKE operations in the where-clause has exactly 1 field and 1 exprval.
		 *
		 * 	SELECT f1 from A WHERE f1 LIKE 'bla; (okay)
		 * 	SELECT f1 from A WHERE f1 LIKE f1; (invalid)
		 * 	SELECT f1 from A WHERE 1 LIKE f1; (invalid)
		 * 	SELECT f1 from A WHERE f1 LIKE 1; (invalid)
		 * 	SELECT f1 from A WHERE 1 LIKE 1; (invalid)
		 */
		if (!check_where_clause_like(where_node, out_err, out_err_len))
			return false;

		/*
		 * check if there occurences of COUNT() function via alias or directly.
		 *
		 * 	SELECT COUNT(*) as val FROM A WHERE val > 1; (invalid);
		 * 	SELECT COUNT(*) FROM A WHERE COUNT(*)> 1; (invalid);
		 */
		if (!check_where_clause_count(root, out_err, out_err_len, column_alias))
			return false;
	}

	return true;
}

static bool check_select_clause(struct ast_node *root, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;

	if (root->node_type != AST_TYPE_SEL_SELECT
			&& root->node_type != AST_TYPE_SEL_SELECTALL
			&& root->node_type != AST_TYPE_SEL_EXPROP
			&& root->node_type != AST_TYPE_SEL_EXPRVAL
			&& root->node_type != AST_TYPE_SEL_FIELDNAME
			&& root->node_type != AST_TYPE_SEL_COUNT
			&& root->node_type != AST_TYPE_SEL_ALIAS) {

		snprintf(out_err,
				out_err_len,
				"only columns, aliases, functions and recursive expressions are valid in select clause\n");
		return false;
	} else {
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (tmp_entry->node_type == AST_TYPE_SEL_WHERE
					|| tmp_entry->node_type == AST_TYPE_SEL_TABLE
					|| tmp_entry->node_type == AST_TYPE_SEL_JOIN
					|| tmp_entry->node_type == AST_TYPE_SEL_HAVING
					|| tmp_entry->node_type == AST_TYPE_SEL_GROUPBY
					|| tmp_entry->node_type == AST_TYPE_SEL_ORDERBYLIST
					|| tmp_entry->node_type == AST_TYPE_SEL_LIMIT) {
				continue;
			}

			if (!check_select_clause(tmp_entry, out_err, out_err_len))
				return false;
		}
	}
	return true;
}

static bool check_groupby_clause_expr(struct ast_node *node, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_exprval_node *valn;

	list_for_each(pos, node->node_children_head)
	{
		tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

		if (tmp_entry->node_type == AST_TYPE_SEL_EXPRVAL) {
			valn = (typeof(valn))tmp_entry;

			if (!valn->value_type.is_name) {
				snprintf(out_err, out_err_len,
						"group-by clauses support only fields and aliases\n");
				return false;
			}
		} else if (tmp_entry->node_type != AST_TYPE_SEL_FIELDNAME) {
			snprintf(out_err, out_err_len,
					"group-by clauses support only fields and aliases\n");
			return false;
		}
	}

	return true;
}

static bool check_groupby_clause_count(struct ast_node *root, char *out_err, size_t out_err_len, struct hashtable *column_alias)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_exprval_node *val_node;
	struct hashtable_value *ht_value;
	struct alias_value *alias_value;

	if (root->node_type == AST_TYPE_SEL_COUNT) {
		snprintf(out_err, out_err_len, "COUNT function can't be used in the groupby-clause\n");
		return false;
	} else if (root->node_type == AST_TYPE_SEL_EXPRVAL) {
		val_node = (typeof(val_node))root;
		if (val_node->value_type.is_name) {
			ht_value = hashtable_get(column_alias, val_node->name_val, strlen(val_node->name_val) + 1);
			if (ht_value) {
				alias_value = (typeof(alias_value))ht_value->content;
				if (alias_value->type.is_count) {
					snprintf(out_err,
							out_err_len,
							"COUNT function can't be used in the groupby-clause\n");
					return false;
				}
			}
		}

	} else {
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_groupby_clause_count(tmp_entry, out_err, out_err_len, column_alias))
				return false;
		}
	}
	return true;
}

static bool is_node_in_select_list(struct ast_node *root, struct ast_node *node)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_exprval_node *val_node;
	struct ast_sel_fieldname_node *field_node;
	struct ast_sel_alias_node *alias_node;

	/* base cases */
	if (!in_select_clause(root)
			|| root->node_type == AST_TYPE_SEL_COUNT
			|| root->node_type == AST_TYPE_SEL_LOGOP) {
		return false;
	}
	else if (root->node_type == AST_TYPE_SEL_EXPRVAL && root->node_type == node->node_type) {
		val_node = (typeof(val_node))root;

		if (val_node->value_type.is_name) {
			return strcmp(val_node->name_val, ((typeof(val_node))node)->name_val) == 0;
		}
	}
	else if (root->node_type == AST_TYPE_SEL_FIELDNAME && root->node_type == node->node_type) {
		field_node = (typeof(field_node))root;
		return strcmp(field_node->table_name, ((typeof(field_node))node)->table_name) == 0
				&& strcmp(field_node->col_name, ((typeof(field_node))node)->col_name) == 0;
	}
	else if (root->node_type == AST_TYPE_SEL_ALIAS && node->node_type == AST_TYPE_SEL_EXPRVAL) {
		alias_node = (typeof(alias_node))root;
		return strcmp(alias_node->alias_value, ((typeof(val_node))node)->name_val) == 0;
	}
	/* recursion */
	else {
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (is_node_in_select_list(tmp_entry, node))
				return true;
		}
	}

	return false;
}

static bool check_groupby_clause_inselect(struct ast_node *root, struct ast_node *node, char *out_err,
		size_t out_err_len)
{

	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_fieldname_node *field_node;
	struct ast_sel_exprval_node *val_node;

	/* base case 1 */
	if (node->node_type == AST_TYPE_SEL_EXPRVAL) {
		val_node = (typeof(val_node))node;

		if (val_node->value_type.is_name) {
			if (!is_node_in_select_list(root, node)) {
				snprintf(out_err, out_err_len, "SELECT list is not in GROUP BY clause and contains "
						"nonaggregated column: '%.128s'\n",
						val_node->name_val);
				return false;
			}
		} else {
			/* something went really wrong here */
			BUG_GENERIC();
		}
	}
	/* base case 2 */
	else if (node->node_type == AST_TYPE_SEL_FIELDNAME) {
		field_node = (typeof(field_node))node;

		if (!is_node_in_select_list(root, node)) {
			snprintf(out_err, out_err_len, "SELECT list is not in GROUP BY clause and contains "
					"nonaggregated column: '%.128s'.'%.128s'\n",
					field_node->table_name, field_node->col_name);
			return false;
		}

	}
	/* recursion  */
	else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_groupby_clause_inselect(root, tmp_entry, out_err, out_err_len))
				return false;
		}
	}

	return true;
}

static bool check_groupby_clause(struct ast_node *root, char *out_err, size_t out_err_len, struct hashtable *column_alias)
{
	struct ast_node *groupby_node;

	groupby_node = find_node(root, AST_TYPE_SEL_GROUPBY);

	if (groupby_node) {
		/*
		 * check if EXPRVAL (names) in the group by clause are either columns or alias... Also ensure that none of them
		 * are ambiguous if more than 1 table is used. This should handle cases like:
		 *
		 * 	CREATE TABLE A (f1 INT);
		 *
		 *	SELECT f1 FROM A GROUP BY f1; (okay)
		 *	SELECT A.f1 FROM A GROUP BY A.f1; (okay):
		 * 	SELECT f1 FROM A GROUP BY f2 + 2; (only fields and alias are supported)
		 * 	SELECT f1 FROM A GROUP BY 2; (only fields and alias are supported)
		 */
		if (!check_groupby_clause_expr(groupby_node, out_err, out_err_len))
			return false;

		/* check if there occurrences of COUNT() function via alias or directly */
		if (!check_groupby_clause_count(groupby_node, out_err, out_err_len, column_alias))
			return false;

		/* check if fields/aliases on group-by clause are also specified on the SELECT list */
		if (!check_groupby_clause_inselect(root, groupby_node, out_err, out_err_len))
			return false;
	}

	return true;
}

static bool check_orderby_clause_expr(struct ast_node *node, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_exprval_node *valn;

	if (node->node_type == AST_TYPE_SEL_ORDERBYITEM) {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (tmp_entry->node_type == AST_TYPE_SEL_EXPRVAL) {
				valn = (typeof(valn))tmp_entry;

				if (!valn->value_type.is_name) {
					snprintf(out_err, out_err_len,
							"order-by clauses support only fields and aliases\n");
					return false;
				}
			} else if (tmp_entry->node_type != AST_TYPE_SEL_FIELDNAME) {
				snprintf(out_err, out_err_len,
						"order-by clauses support only fields and aliases\n");
				return false;
			}
		}

	} else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_orderby_clause_expr(tmp_entry, out_err, out_err_len))
				return false;
		}
	}

	return true;

}

static bool check_orderby_clause_count(struct ast_node *root, char *out_err, size_t out_err_len, struct hashtable *column_alias)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_exprval_node *val_node;
	struct hashtable_value *ht_value;
	struct alias_value *alias_value;

	if (root->node_type == AST_TYPE_SEL_COUNT) {
		snprintf(out_err, out_err_len, "COUNT function can't be used in the orderby-clause\n");
		return false;
	} else if (root->node_type == AST_TYPE_SEL_EXPRVAL) {
		val_node = (typeof(val_node))root;
		if (val_node->value_type.is_name) {
			ht_value = hashtable_get(column_alias, val_node->name_val, strlen(val_node->name_val) + 1);
			if (ht_value) {
				alias_value = (typeof(alias_value))ht_value->content;
				if (alias_value->type.is_count) {
					snprintf(out_err,
							out_err_len,
							"COUNT function can't be used in the orderby-clause\n");
					return false;
				}
			}
		}

	} else {
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_orderby_clause_count(tmp_entry, out_err, out_err_len, column_alias))
				return false;
		}
	}
	return true;
}

static bool check_orderby_clause(struct ast_node *root, char *out_err, size_t out_err_len, struct hashtable *column_alias)
{
	struct ast_node *orderby_node;

	orderby_node = find_node(root, AST_TYPE_SEL_ORDERBYLIST);

	if (orderby_node) {
		/*
		 * check if EXPRVAL (names) in the group by clause are either columns or alias... Also ensure that none of them
		 * are ambiguous if more than 1 table is used. This should handle cases like:
		 *
		 * 	CREATE TABLE A (f1 INT);
		 *
		 *	SELECT f1 FROM A ORDER BY f1; (okay)
		 *	SELECT A.f1 FROM A ORDER BY A.f1; (okay):
		 * 	SELECT f1 FROM A ORDER BY f2 + 2; (only fields and alias are supported)
		 * 	SELECT f1 FROM A ODER BY 2; (only fields and alias are supported)
		 */
		if (!check_orderby_clause_expr(orderby_node, out_err, out_err_len))
			return false;

		/* check if there occurrences of COUNT() function via alias or directly */
		if (!check_orderby_clause_count(root, out_err, out_err_len, column_alias))
			return false;
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

	/*
	 * build column alias hashtable; check for column alias validity
	 *
	 * Column Alias challenges:
	 *
	 * - SELECT f1 as val FROM A; (simple alias)
	 * - SELECT A.f1 as val FROM A; (full qualified name)
	 * - SELECT x.f1 as val FROM A as x; (table alias + column alias);
	 * - SELECT x.f1 as val, val * 2 as bla FROM A as x; (invalid as val would be considered as a column)
	 * - SELECT (x.f1 * 2) / x.f1 as val FROM A x (recursive expression - this is the hard one apparently);
	 * 	-> alias_value.type.is_expre is set in this case
	 */
	if (!check_column_alias(db, node, out_err, out_err_len, &table_alias, &column_alias))
		goto err_column_alias;

	/*
	 * check columns
	 *
	 * Challenges:
	 *
	 * CREATE TABLE I_D_1 (f1 int);
	 * CREATE TABLE I_D_2 (f2 int);
	 *
	 * - SELECT f2 FROM I_D_1 JOIN I_D_2 ON f1 = f3; - f3 clearly don't exist in I_D_1 or I_D_2... so all good here
	 * - SELECT f2 FROM I_D_1 JOIN I_D_2 ON f1 = f1; - I need a join checker as I can't validate if I'm pointing to
	 * 	fields on both sides of the join.... this can get tricky when accounting for multiple joins
	 * - SELECT f1 as val FROM I_D_1 WHERE val > 2; - Done.
	 * - SELECT f1 as val FROM I_D_1 GROUP BY val; - Done
	 * - SELECT f1 as val FROM I_D_1 ORDER BY val; - Done.
	 * - SELECT count(*) as val FROM I_D_1 HAVING COUNT(*) > 1; - need to check alias on having statements
	 * 	TODO: Figure this out
	 */
	if (!check_column_names(db, node, node, out_err, out_err_len, &table_alias, &column_alias))
		goto err_column_names;

	/*
	 * Check for COUNT functions
	 * This takes care of both SELECT and HAVING clauses. WHERE-clause is processed later as no
	 * COUNT function is allowed in the there.
	 *
	 * 	SELECT COUNT(*) FROM A; (okay)
	 * 	SELECT COUNT(f1) FROM A; (okay)
	 * 	SELECT COUNT(f1 + f2) FROM A; (invalid)
	 * 	SELECT COUNT(1) FROM A; (invalid)
	 * 	SELECT COUNT(B.f1) FROM A; (invalid fqfield - not part of FROM clause)
	 * 	SELECT COUNT(non_valid_column) FROM A; (invalid) -> (taken care of through check_column_names routine)
	 * 	SELECT COUNT(alias_name) FROM A; (invalid)
	 */
	if (!check_count_function(node, out_err, out_err_len))
		return false;

	/* check select to ensure only columns, recursive expressions, COUNT fncs, and aliases are used.
	 * Expr grammar rule is too generic but that seems to be the case for other engines too.
	 * It's preferable to debug this code than to debug bison's parser code instead :) */
	if (!check_select_clause(node, out_err, out_err_len))
		goto err_select_clause;

	/* check where clause */
	if (!check_where_clause(node, out_err, out_err_len, &column_alias))
		goto err_where_clause;

	/*
	 * check if group-by clause:
	 * - contains only exprval (name) / fieldnames / alias (later one is verified in check_column_names)
	 * - fields are part of the SELECT fields
	 * 	-> This is also a different thing on SQLITE and MySQL..... got decide how I want it to work.
	 */
	if (!check_groupby_clause(node, out_err, out_err_len, &column_alias))
		goto err_groupby_clause;

	/*
	 * check if order-by clause contains only exprval (name) / fieldnames / alias (later one is verified in
	 * check_column_names)
	 */
	if (!check_orderby_clause(node, out_err, out_err_len, &column_alias))
		goto err_groupby_clause;

	/*
	 * TODO:
	 * - Check for duplicate / invalid aliases (table done, columns done)
	 * 	-> Need to change implementation though... instead of storing as KV alias->table_name (for table aliases)
	 * 	   and alias->?? (for column), I need to find a common denominator. Otherwise, it will become hard to
	 * 	   validate the existence of columns at some point... my best guess, it to use some structure that resembles
	 * 	   the ast_sel_fieldname_nome...  -> I think I've done it (TBC)
	 * - Check for aliases use in the where clause (also group, order, having).
	 * 	-> This is becoming a big dilemma for me as although column aliases are not available in where clause (MySQL)
	 * 		they seem to be used in group by and order by.... I wonder if I should implement it for all
	 * 		like SQLite does:
	 *
	 * 		ex: SELECT val as f1 FROM A group by f1 order by f1;
	 *
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

err_select_clause:
err_where_clause:
err_groupby_clause:
err_column_names:
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
