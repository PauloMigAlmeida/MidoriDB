/*
 * semantic_select.c
 *
 *  Created on: 10/10/2023
 *      Author: paulo
 */

#define _XOPEN_SOURCE       /* See feature_test_macros(7) */
#include <parser/semantic.h>
#include <primitive/table.h>
#include <primitive/column.h>
#include <datastructure/hashtable.h>
#include <datastructure/linkedlist.h>

static struct sem_check_val_types __check_value_types(struct ast_node *node, struct hashtable *ht, char *out_err,
		size_t out_err_len);

#define FQFIELD_NAME_LEN 	MEMBER_SIZE(struct ast_sel_fieldname_node, table_name)			\
					+ 1 /* dot */ 							\
					+ MEMBER_SIZE(struct ast_sel_fieldname_node, col_name)		\
					+ 1 /* NUL */

#define FIELD_NAME_LEN 		MEMBER_SIZE(struct ast_sel_exprval_node, name_val)			\
					+ 1 /* NUL */

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

/* sanity checks during compilation to ensure logic below holds up */
BUILD_BUG(MEMBER_SIZE(struct alias_value, table_name) == MEMBER_SIZE(struct ast_sel_table_node, table_name),
		"must have the same size");
BUILD_BUG(MEMBER_SIZE(struct alias_value, col_name) == MEMBER_SIZE(struct ast_sel_exprval_node, name_val),
		"must have the same size");
BUILD_BUG(MEMBER_SIZE(struct alias_value, col_name) == MEMBER_SIZE(struct ast_sel_fieldname_node, col_name),
		"must have the same size");

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

static bool column_exists(struct database *db, struct ast_sel_table_node *table_node, char *col_name)
{
	struct table *table;
	table = database_table_get(db, table_node->table_name);

	for (int i = 0; i < table->column_count; i++) {
		if (strcmp(table->columns[i].name, col_name) == 0)
			return true;
	}

	return false;
}

static int tables_with_column_name(struct database *db, struct ast_node *root, char *col_name)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_table_node *table_node;
	int count = 0;

	if (root->node_type == AST_TYPE_SEL_TABLE) {
		table_node = (typeof(table_node))root;

		if (column_exists(db, table_node, col_name))
			count++;
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
		 * 	SELECT COUNT(f1) FROM A HAVING COUNT(f2) > 0; (no such column)
		 */
		if (!check_column_names_having(db, root, having_node, out_err, out_err_len, table_alias,
						column_alias))
			return false;
	}

	return true;
}

static bool check_count_expr(struct ast_node *node, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_exprval_node *val_node;

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
				/* do nothing, we are good */
			} else {
				snprintf(out_err, out_err_len, "COUNT function can only have '*' or 'fields'\n");
				return false;
			}

		}
	} else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_count_expr(tmp_entry, out_err, out_err_len))
				return false;
		}
	}

	return true;
}

static bool check_count_select(struct ast_node *parent, struct ast_node *node, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;

	/* base cases */
	if (!in_select_clause(node)) {
		return true;
	}
	else if (node->node_type == AST_TYPE_SEL_COUNT
			&& parent->node_type != AST_TYPE_SEL_ALIAS
			&& parent->node_type != AST_TYPE_SEL_SELECT) {
		snprintf(out_err, out_err_len, "In SELECT list, COUNT function must either be raw or use aliases\n");
		return false;
	}
	else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_count_select(node, tmp_entry, out_err, out_err_len))
				return false;
		}
	}
	return true;
}

static bool check_count_function(struct ast_node *node, char *out_err, size_t out_err_len)
{
	/* make sure COUNT function only contains *, columns and fqfields */
	if (!check_count_expr(node, out_err, out_err_len))
		return false;

	/* in SELECT list, COUNT can only be at free form or using aliases, that's it.. no fancy stuff */
	if (!check_count_select(node, node, out_err, out_err_len))
		return false;

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
		if (!check_where_clause_count(where_node, out_err, out_err_len, column_alias))
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

static bool check_join_on_expr(struct ast_node *node, struct ast_node *parent, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;

	if ((node->node_type == AST_TYPE_SEL_EXPRVAL || node->node_type == AST_TYPE_SEL_FIELDNAME)
			&& parent->node_type != AST_TYPE_SEL_CMP) {
		snprintf(out_err, out_err_len, "JOIN expressions support only logical comparisons\n");
		return false;
	} else if (node->node_type == AST_TYPE_SEL_CMP
			&& parent->node_type != AST_TYPE_SEL_LOGOP
			&& parent->node_type != AST_TYPE_SEL_ONEXPR) {
		snprintf(out_err, out_err_len, "JOIN expressions support only logical comparisons\n");
		return false;
	} else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_join_on_expr(tmp_entry, node, out_err, out_err_len))
				return false;
		}
	}

	return true;
}

static bool check_join_on_count(struct ast_node *node, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;

	if (node->node_type == AST_TYPE_SEL_COUNT) {
		snprintf(out_err, out_err_len, "COUNT() functions are not valid in JOIN ON expressions\n");
		return false;
	} else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_join_on_count(tmp_entry, out_err, out_err_len))
				return false;
		}
	}

	return true;
}

static int check_join_on_fields_exprname(struct database *db, struct ast_sel_exprval_node *val_node,
		struct ast_sel_join_node *join_node, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_join_node *new_join_node;
	struct ast_sel_table_node *table_node;
	int count = 0;

	list_for_each(pos, join_node->node_children_head)
	{
		tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

		if (tmp_entry->node_type == AST_TYPE_SEL_TABLE) {
			table_node = (typeof(table_node))tmp_entry;

			if (column_exists(db, table_node, val_node->name_val))
				count++;

		} else if (tmp_entry->node_type == AST_TYPE_SEL_JOIN) {
			new_join_node = (typeof(new_join_node))tmp_entry;
			count += check_join_on_fields_exprname(db, val_node, new_join_node, out_err, out_err_len);
		}
	}

	return count;
}

static int check_join_on_fields_fieldname(struct database *db, struct ast_sel_fieldname_node *field_node,
		struct ast_sel_join_node *join_node, char *out_err, size_t out_err_len)
{
	struct list_head *pos1, *pos2;
	struct ast_node *tmp_entry;
	struct ast_sel_join_node *new_join_node;
	struct ast_sel_table_node *table_node;
	struct ast_sel_alias_node *alias_node;
	int count = 0;

	list_for_each(pos1, join_node->node_children_head)
	{
		tmp_entry = list_entry(pos1, typeof(*tmp_entry), head);

		/* fqfield */
		if (tmp_entry->node_type == AST_TYPE_SEL_TABLE) {
			table_node = (typeof(table_node))tmp_entry;

			/* is it the table that we are looking for ? */
			if (strcmp(table_node->table_name, field_node->table_name) != 0)
				continue;

			if (column_exists(db, table_node, field_node->col_name))
				count++;

		}
		/* fields using aliases */
		else if (tmp_entry->node_type == AST_TYPE_SEL_ALIAS) {
			alias_node = (typeof(alias_node))tmp_entry;

			/* is it the table that we are looking for ? */
			if (strcmp(alias_node->alias_value, field_node->table_name) != 0)
				continue;

			list_for_each(pos2, alias_node->node_children_head)
			{
				table_node = list_entry(pos2, typeof(*table_node), head);

				if (column_exists(db, table_node, field_node->col_name))
					count++;
			}

		} else if (tmp_entry->node_type == AST_TYPE_SEL_JOIN) {
			new_join_node = (typeof(new_join_node))tmp_entry;
			count += check_join_on_fields_fieldname(db, field_node, new_join_node, out_err, out_err_len);
		}
	}

	return count;
}

static bool check_join_on_fields(struct database *db, struct ast_node *node, struct ast_sel_join_node *join_node,
		char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_join_node *new_join_node;
	struct ast_sel_exprval_node *val_node;
	struct ast_sel_fieldname_node *field_node;
	int count;

	if (node->node_type == AST_TYPE_SEL_EXPRVAL) {
		val_node = (typeof(val_node))node;

		if (val_node->value_type.is_name) {
			count = check_join_on_fields_exprname(db, val_node, join_node, out_err, out_err_len);

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

		count = check_join_on_fields_fieldname(db, field_node, join_node, out_err, out_err_len);

		if (count == 0) {
			snprintf(out_err, out_err_len, "no such column: '%.128s.%.128s'\n",
					field_node->table_name,
					field_node->col_name);
			return false;
		}
	} else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (tmp_entry->node_type == AST_TYPE_SEL_JOIN)
				new_join_node = (typeof(new_join_node))tmp_entry;
			else
				new_join_node = join_node;

			if (!check_join_on_fields(db, tmp_entry, new_join_node, out_err, out_err_len))
				return false;

		}
	}

	return true;
}

static bool check_from_clause(struct database *db, struct ast_node *root, char *out_err, size_t out_err_len)
{
	struct ast_node *join_node;

	join_node = find_node(root, AST_TYPE_SEL_JOIN);

	if (join_node) {

		if (!check_join_on_expr(join_node, root, out_err, out_err_len))
			return false;

		/* check if there occurrences of COUNT() function */
		if (!check_join_on_count(join_node, out_err, out_err_len))
			return false;

		/* check if fields/aliases on JOIN ON expression */
		if (!check_join_on_fields(db, root, (struct ast_sel_join_node*)join_node, out_err, out_err_len))
			return false;
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

static bool is_node_in_groupby_clause(struct ast_node *node, struct ast_node *groupby_node)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_exprval_node *val_node;
	struct ast_sel_fieldname_node *field_node;
	struct ast_sel_alias_node *alias_node;

	list_for_each(pos, groupby_node->node_children_head)
	{
		tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

		if (node->node_type == AST_TYPE_SEL_EXPRVAL && tmp_entry->node_type == node->node_type) {
			val_node = (typeof(val_node))node;

			if (val_node->value_type.is_name) {
				if (strcmp(val_node->name_val, ((typeof(val_node))tmp_entry)->name_val) == 0)
					return true;
			}
		}
		else if (node->node_type == AST_TYPE_SEL_FIELDNAME && tmp_entry->node_type == node->node_type) {
			field_node = (typeof(field_node))node;

			if (strcmp(field_node->table_name, ((typeof(field_node))tmp_entry)->table_name) == 0
					&& strcmp(field_node->col_name, ((typeof(field_node))tmp_entry)->col_name) == 0)
				return true;
		}
		else if (node->node_type == AST_TYPE_SEL_ALIAS && tmp_entry->node_type == AST_TYPE_SEL_EXPRVAL) {
			alias_node = (typeof(alias_node))node;
			if (strcmp(alias_node->alias_value, ((typeof(val_node))tmp_entry)->name_val) == 0)
				return true;
		}
	}

	return false;
}

static bool check_groupby_clause_inselect(struct ast_node *root, struct ast_node *groupby_node, char *out_err,
		size_t out_err_len, struct hashtable *column_alias)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_alias_node *alias_node;
	struct hashtable_value *ht_value;
	struct alias_value *alias_value;

	/* base cases */
	if (!in_select_clause(root)
			|| root->node_type == AST_TYPE_SEL_COUNT
			|| root->node_type == AST_TYPE_SEL_LOGOP) {
		return true;
	}
	else if (root->node_type == AST_TYPE_SEL_EXPRVAL
			|| root->node_type == AST_TYPE_SEL_FIELDNAME) {
		return is_node_in_groupby_clause(root, groupby_node);
	}
	else if (root->node_type == AST_TYPE_SEL_ALIAS) {
		alias_node = (typeof(alias_node))root;

		ht_value = hashtable_get(column_alias, alias_node->alias_value, strlen(alias_node->alias_value) + 1);

		if (!ht_value) {
			/* it must be a table alias in which case we are not interested in it.. skip it */
			return true;
		} else {
			alias_value = (typeof(alias_value))ht_value->content;

			/* alias to a COUNT function, makes no sense to check if that is in the group-by clause */
			if (alias_value->type.is_count)
				return true;
		}

		return is_node_in_groupby_clause(root, groupby_node);
	}
	/* recursion */
	else {
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_groupby_clause_inselect(tmp_entry, groupby_node, out_err, out_err_len, column_alias))
				return false;
		}
	}

	return true;
}

struct sem_sel_fds_stats {
	int no_raw_fields;
	int no_count_fnc;
};

static void check_aggr_inselect_nogroupby(struct ast_node *root, char *out_err, size_t out_err_len,
		struct hashtable *column_alias, struct sem_sel_fds_stats *out_stats)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_alias_node *alias_node;
	struct hashtable_value *ht_value;
	struct alias_value *alias_value;

	/* base cases */
	if (!in_select_clause(root)) {
		return;
	}
	else if (root->node_type == AST_TYPE_SEL_COUNT) {
		out_stats->no_count_fnc++;
	}
	else if (root->node_type == AST_TYPE_SEL_EXPRVAL
			|| root->node_type == AST_TYPE_SEL_EXPROP
			|| root->node_type == AST_TYPE_SEL_FIELDNAME) {
		out_stats->no_raw_fields++;
	}
	else if (root->node_type == AST_TYPE_SEL_ALIAS) {
		alias_node = (typeof(alias_node))root;

		ht_value = hashtable_get(column_alias, alias_node->alias_value, strlen(alias_node->alias_value) + 1);

		if (ht_value) {
			alias_value = (typeof(alias_value))ht_value->content;

			/* alias to a COUNT function, makes no sense to check if that is in the group-by clause */
			if (alias_value->type.is_count)
				out_stats->no_count_fnc++;
			else
				out_stats->no_raw_fields++;
		}
	}
	/* recursion */
	else {
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			check_aggr_inselect_nogroupby(tmp_entry, out_err, out_err_len, column_alias, out_stats);
		}
	}
}

static bool check_groupby_clause(struct ast_node *root, char *out_err, size_t out_err_len, struct hashtable *column_alias)
{
	struct ast_node *groupby_node;
	struct sem_sel_fds_stats out_stats = {0};

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
		if (!check_groupby_clause_inselect(root, groupby_node, out_err, out_err_len, column_alias))
			return false;
	} else {
		/*
		 * edge case: non-aggregate fields alongside aggregate fields with no group-by clause such as:
		 * 	SELECT f1, COUNT(f2) FROM B;  (should fail as f1 must be in the group-by clause)
		 */
		check_aggr_inselect_nogroupby(root, out_err, out_err_len, column_alias, &out_stats);
		if (out_stats.no_count_fnc > 0 && out_stats.no_raw_fields > 0)
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
			ht_value = hashtable_get(column_alias, val_node->name_val,
							strlen(val_node->name_val) + 1);
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

static bool check_orderby_clause_inselect(struct ast_node *root, struct ast_node *node, char *out_err,
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
				snprintf(out_err, out_err_len,
						"SELECT list is not in ORDER BY clause: '%.128s'\n",
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
			snprintf(out_err, out_err_len,
					"SELECT list is not in ORDER BY clause: '%.128s'.'%.128s'\n",
					field_node->table_name,
					field_node->col_name);
			return false;
		}

	}
	/* recursion  */
	else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_orderby_clause_inselect(root, tmp_entry, out_err, out_err_len))
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

		/* check if fields/aliases on order-by clause are also specified on the SELECT list */
		if (!check_orderby_clause_inselect(root, orderby_node, out_err, out_err_len))
			return false;
	}

	return true;
}

static bool check_having_clause_expr(struct ast_node *root, struct ast_node *parent, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;

	if (root->node_type == AST_TYPE_SEL_EXPRVAL || root->node_type == AST_TYPE_SEL_EXPROP) {
		if (parent->node_type == AST_TYPE_SEL_HAVING || parent->node_type == AST_TYPE_SEL_LOGOP) {
			snprintf(out_err, out_err_len,
					"expressions in having-clause must be a type of comparison\n");
			return false;
		}
	} else {
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_having_clause_expr(tmp_entry, root, out_err, out_err_len))
				return false;
		}
	}
	return true;
}

static bool check_having_clause_inselect(struct ast_node *root, struct ast_node *node, char *out_err,
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
				snprintf(out_err, out_err_len,
						"SELECT list is not in HAVING clause: '%.128s'\n",
						val_node->name_val);
				return false;
			}
		}
	}
	/* base case 2 */
	else if (node->node_type == AST_TYPE_SEL_FIELDNAME) {
		field_node = (typeof(field_node))node;

		if (!is_node_in_select_list(root, node)) {
			snprintf(out_err, out_err_len,
					"SELECT list is not in HAVING clause: '%.128s'.'%.128s'\n",
					field_node->table_name,
					field_node->col_name);
			return false;
		}

	}
	/* base case 3 - COUNT has a special treatment on having-clauses */
	else if (node->node_type == AST_TYPE_SEL_COUNT) {
		return true;
	}
	/* recursion  */
	else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_having_clause_inselect(root, tmp_entry, out_err, out_err_len))
				return false;
		}
	}

	return true;
}

static bool check_having_clause(struct ast_node *root, char *out_err, size_t out_err_len, struct hashtable *column_alias)
{
	UNUSED(out_err);
	UNUSED(out_err_len);
	UNUSED(column_alias);

	struct ast_node *having_node;

	having_node = find_node(root, AST_TYPE_SEL_HAVING);

	if (having_node) {
		/*
		 * check if EXPRVAL (names) in the group by clause are either columns or alias... Also ensure that none of them
		 * are ambiguous if more than 1 table is used. This should handle cases like:
		 *
		 * 	CREATE TABLE A (f1 INT);
		 *
		 *	SELECT f1 FROM A HAVING f1 > 0; (okay)
		 *	SELECT A.f1 FROM A HAVING A.f1 > 1; (okay):
		 * 	SELECT f1 FROM A HAVING f2 + 2; (only fields and alias are supported)
		 * 	SELECT f1 FROM A HAVING 2; (only fields and alias are supported)
		 */
		if (!check_having_clause_expr(having_node, NULL, out_err, out_err_len))
			return false;

		/* check if fields/aliases on order-by clause are also specified on the SELECT list */
		if (!check_having_clause_inselect(root, having_node, out_err, out_err_len))
			return false;
	}

	return true;
}

static bool check_isxin_entries(struct ast_node *root, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_sel_isxin_node *isxin_node;
	struct ast_sel_exprval_node *val_node;
	struct ast_node *tmp_entry;
	int field_count;

	// base case
	if (root->node_type == AST_TYPE_SEL_EXPRISXIN) {
		isxin_node = (typeof(isxin_node))root;

		field_count = 0;
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (tmp_entry->node_type == AST_TYPE_SEL_EXPRVAL) {
				val_node = (typeof(val_node))tmp_entry;
				if (val_node->value_type.is_name) {
					field_count++;
				}
			} else if (tmp_entry->node_type == AST_TYPE_SEL_FIELDNAME) {
				field_count++;
			} else {
				snprintf(out_err, out_err_len, "IN-clause can only contain raw values\n");
				return false;
			}

			/*
			 * ast_sel_isxin_node holds the field which is part of the expression in the
			 * linked-list. So it's expected to have 1 field but no more than that.
			 */
			if (field_count > 1) {
				snprintf(out_err, out_err_len, "Fields aren't allowed on IN-clauses\n");
				return false;
			}
		}
	} else {
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);
			if (!check_isxin_entries(tmp_entry, out_err, out_err_len))
				return false;
		}
	}

	return true;
}

static bool check_isxnull_entries(struct ast_node *root, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_sel_isxnull_node *isxnull_node;
	struct ast_sel_exprval_node *val_node;
	struct ast_node *tmp_entry;

	// base case
	if (root->node_type == AST_TYPE_SEL_EXPRISXNULL) {
		isxnull_node = (typeof(isxnull_node))root;

		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (tmp_entry->node_type == AST_TYPE_SEL_EXPRVAL) {
				val_node = (typeof(val_node))tmp_entry;

				if (!val_node->value_type.is_name) {
					snprintf(out_err, out_err_len,
							"only fields are allowed in IS NULL|IS NOT NULL\n");
					return false;
				}
			} else if (tmp_entry->node_type != AST_TYPE_SEL_FIELDNAME) {
				/* something I haven't foreseen */
				BUG_GENERIC();
			}

		}
	} else {
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);
			if (!check_isxnull_entries(tmp_entry, out_err, out_err_len))
				return false;
		}
	}

	return true;
}

struct sem_check_val_types {
	bool none;
	bool invalid;
	bool null;
	enum COLUMN_TYPE type;
};

static struct sem_check_val_types check_value_types_cmp(struct ast_sel_cmp_node *cmp_node, struct hashtable *ht, char *out_err, size_t out_err_len)
{
	struct sem_check_val_types ret = {0};
	struct sem_check_val_types left = {0};
	struct sem_check_val_types right = {0};
	int i = 0;

	struct ast_node *tmp_entry = NULL;
	struct list_head *pos;

	/* sanity check */
	BUG_ON(list_length(cmp_node->node_children_head) != 2);

	list_for_each(pos, cmp_node->node_children_head)
	{
		tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

		if (i == 0) {
			left = __check_value_types(tmp_entry, ht, out_err, out_err_len);
		} else {
			right = __check_value_types(tmp_entry, ht, out_err, out_err_len);
		}
		i++;
	}

	// NULL has a special set of rules
	if (left.null || right.null) {
		if (cmp_node->cmp_type != AST_CMP_EQUALS_OP && cmp_node->cmp_type != AST_CMP_DIFF_OP) {
			snprintf(out_err, out_err_len, "NULL values can only use '=' or '<>' ops\n");
			ret.invalid = true;
		} else {
			ret.type = CT_TINYINT;
		}
		return ret;
	}

	// VARCHAR has a special set of rules
	if (left.type == CT_VARCHAR || right.type == CT_VARCHAR) {
		if (cmp_node->cmp_type != AST_CMP_EQUALS_OP && cmp_node->cmp_type != AST_CMP_DIFF_OP) {
			snprintf(out_err, out_err_len, "VARCHAR values can only use '=' or '<>' ops\n");
			ret.invalid = true;
		}
	}

	if (memcmp(&left, &right, sizeof(left))) {
		ret.invalid = true;
	} else {
		ret.type = CT_TINYINT; // CMP resolves into a Boolean value
	}

	return ret;
}

static struct sem_check_val_types check_value_types_exprop(struct ast_sel_exprop_node *node, struct hashtable *ht, char *out_err, size_t out_err_len)
{
	struct sem_check_val_types ret = {0};
	struct sem_check_val_types left = {0};
	struct sem_check_val_types right = {0};
	int i = 0;

	struct ast_node *tmp_entry = NULL;
	struct list_head *pos;

	/* sanity check */
	BUG_ON(list_length(node->node_children_head) != 2);

	list_for_each(pos, node->node_children_head)
	{
		tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

		if (i == 0) {
			left = __check_value_types(tmp_entry, ht, out_err, out_err_len);
		} else {
			right = __check_value_types(tmp_entry, ht, out_err, out_err_len);
		}
		i++;
	}

	if (memcmp(&left, &right, sizeof(left))) {
		ret.invalid = true;
	} else {
		ret = left;
	}
	return ret;
}

static struct sem_check_val_types check_value_types_logop(struct ast_sel_logop_node *node, struct hashtable *ht, char *out_err, size_t out_err_len)
{
	struct sem_check_val_types ret = {0};
	struct sem_check_val_types left = {0};
	struct sem_check_val_types right = {0};
	int i = 0;

	struct ast_node *tmp_entry = NULL;
	struct list_head *pos;

	/* sanity check */
	BUG_ON(list_length(node->node_children_head) != 2);

	list_for_each(pos, node->node_children_head)
	{
		tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

		if (i == 0) {
			left = __check_value_types(tmp_entry, ht, out_err, out_err_len);
		} else {
			right = __check_value_types(tmp_entry, ht, out_err, out_err_len);
		}
		i++;
	}

	if (memcmp(&left, &right, sizeof(left))) {
		ret.invalid = true;
	} else {
		ret.type = CT_TINYINT; // LOGOP resolves into a Boolean value
	}
	return ret;
}

static struct sem_check_val_types check_value_types_fieldname(struct ast_sel_fieldname_node *field_node, struct hashtable *ht, char *out_err, size_t out_err_len)
{
	UNUSED(out_err);
	UNUSED(out_err_len);

	struct sem_check_val_types ret = {0};
	struct hashtable_value *ht_value;
	char key[FQFIELD_NAME_LEN] = {0};

	snprintf(key, sizeof(key) - 1, "%s.%s", field_node->table_name, field_node->col_name);
	ht_value = hashtable_get(ht, key, strlen(key) + 1);
	BUG_ON(!ht_value); // sanity check
	ret.type = *(enum COLUMN_TYPE*)ht_value->content;

	return ret;
}

static struct sem_check_val_types check_value_types_isxin(struct ast_sel_isxin_node *node, struct hashtable *ht, char *out_err, size_t out_err_len)
{
	struct sem_check_val_types ret = {0};
	struct hashtable_value *ht_value = NULL;
	struct ast_sel_exprval_node *val_node;
	struct ast_sel_fieldname_node *field_node;
	char key[FQFIELD_NAME_LEN] = {0};

	struct ast_node *tmp_entry = NULL;
	struct list_head *pos;

	enum COLUMN_TYPE expected_type;
	bool first = true;

	list_for_each(pos, node->node_children_head)
	{
		tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

		if (first) {
			first = false;

			if (tmp_entry->node_type == AST_TYPE_SEL_EXPRVAL) {
				val_node = (typeof(val_node))tmp_entry;
				ht_value = hashtable_get(ht, val_node->name_val, strlen(val_node->name_val) + 1);
			} else if (tmp_entry->node_type == AST_TYPE_SEL_FIELDNAME) {
				field_node = (typeof(field_node))tmp_entry;
				snprintf(key, sizeof(key) - 1, "%s.%s", field_node->table_name, field_node->col_name);
				ht_value = hashtable_get(ht, key, strlen(key) + 1);
			}

			BUG_ON(!ht_value); // sanity check
			expected_type = *(enum COLUMN_TYPE*)ht_value->content;
		} else {
			// sanity check
			BUG_ON(tmp_entry->node_type != AST_TYPE_SEL_EXPRVAL);
			val_node = (typeof(val_node))tmp_entry;

			if (val_node->value_type.is_str) {
				// strings can be interpreted/parsed for other types such as DATE and DATETIME
				if (expected_type == CT_DATE || expected_type == CT_DATETIME) {
					if (!try_parse_date_type(val_node->str_val, expected_type)) {
						snprintf(out_err,
								out_err_len,
								/* max str size would exceed buffer's size, so trim it */
								"val: '%.256s' can't be parsed for DATE | DATETIME column\n",
								val_node->str_val);
						goto err;
					}
				} else if (expected_type != CT_VARCHAR) {
					snprintf(out_err, out_err_len,
							/* max str size would exceed buffer's size, so trim it */
							"val: '%.256s' requires an VARCHAR() column\n",
							val_node->str_val);
					goto err;
				}
			} else if (val_node->value_type.is_intnum) {
				if (expected_type != CT_INTEGER) {
					snprintf(out_err, out_err_len, "val: '%ld' requires an INTEGER column\n",
							val_node->int_val);
					goto err;
				}
			} else if (val_node->value_type.is_approxnum) {
				if (expected_type != CT_DOUBLE) {
					snprintf(out_err, out_err_len, "val: '%f' requires a DOUBLE column\n",
							val_node->double_val);
					goto err;
				}
			} else if (val_node->value_type.is_bool) {
				if (expected_type != CT_TINYINT) {
					snprintf(out_err, out_err_len, "val: '%d' requires a TINYINT column\n",
							val_node->bool_val);
					goto err;
				}
			} else {
				/* maybe new column type? */
				BUG_GENERIC();
			}
		}
	}

	ret.type = CT_TINYINT; // ISXIN is just a syntax sugar for a set of comparisons (CMP) so it evals to bool
	return ret;

err:
	ret.invalid = true;
	return ret;
}

static struct sem_check_val_types __check_value_types(struct ast_node *node, struct hashtable *ht, char *out_err, size_t out_err_len)
{

	struct sem_check_val_types ret = {0};

	struct ast_node *tmp_entry = NULL;
	struct list_head *pos;

	struct ast_sel_exprval_node *val_node;
	struct hashtable_value *ht_value;

	// deep first search - base case
	if (node->node_type == AST_TYPE_SEL_EXPRVAL) {
		val_node = (typeof(val_node))node;

		if (val_node->value_type.is_name) {
			ht_value = hashtable_get(ht, val_node->name_val, strlen(val_node->name_val) + 1);
			BUG_ON(!ht_value); // sanity check
			ret.type = *(enum COLUMN_TYPE*)ht_value->content;
		} else if (val_node->value_type.is_approxnum) {
			ret.type = CT_DOUBLE;
		} else if (val_node->value_type.is_bool) {
			ret.type = CT_TINYINT;
		} else if (val_node->value_type.is_intnum) {
			ret.type = CT_INTEGER;
		} else if (val_node->value_type.is_str) {
			ret.type = CT_VARCHAR;
		} else if (val_node->value_type.is_null) {
			ret.null = true;
		} else {
			ret.none = true;
		}

		return ret;
	} else if (node->node_type == AST_TYPE_SEL_FIELDNAME) {
		return check_value_types_fieldname((struct ast_sel_fieldname_node*)node, ht, out_err, out_err_len);
	} else if (node->node_type == AST_TYPE_SEL_EXPROP) {
		return check_value_types_exprop((struct ast_sel_exprop_node*)node, ht, out_err, out_err_len);
	} else if (node->node_type == AST_TYPE_SEL_CMP) {
		return check_value_types_cmp((struct ast_sel_cmp_node*)node, ht, out_err, out_err_len);
	} else if (node->node_type == AST_TYPE_SEL_LOGOP) {
		return check_value_types_logop((struct ast_sel_logop_node*)node, ht, out_err, out_err_len);
	} else if (node->node_type == AST_TYPE_SEL_COUNT) {
		ret.type = CT_INTEGER;
		return ret;
	} else if (node->node_type == AST_TYPE_SEL_EXPRISXIN) {
		return check_value_types_isxin((struct ast_sel_isxin_node*)node, ht, out_err, out_err_len);
	}

	// recursion
	else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			ret = __check_value_types(tmp_entry, ht, out_err, out_err_len);

			if (ret.invalid) {
				return ret;
			}
		}
	}

	ret.none = true;
	return ret;
}

static bool build_column_value_types_ht(struct database *db, struct ast_node *node, struct ast_node *parent, struct hashtable *ht, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_table_node *table_node;
	struct ast_sel_alias_node *alias_node;
	struct table *table;
	char key[FQFIELD_NAME_LEN];

	if (node->node_type == AST_TYPE_SEL_TABLE) {
		table_node = (typeof(table_node))node;
		table = database_table_get(db, table_node->table_name);

		for (int i = 0; i < table->column_count; i++) {
			char *col_name = table->columns[i].name;
			enum COLUMN_TYPE col_type = table->columns[i].type;

			/* add exprval (name) or fieldname as it shows up in the query */
			memzero(key, sizeof(key));

			if (parent->node_type == AST_TYPE_SEL_ALIAS) {
				alias_node = (typeof(alias_node))parent;

				snprintf(key, sizeof(key) - 1, "%s.%s", alias_node->alias_value, col_name);
				if (!hashtable_put(ht, key, strlen(key) + 1, &col_type, sizeof(col_type))) {
					snprintf(out_err, out_err_len, "duplicate column name: '%s'\n", key);
					return false;
				}

			} else {
				/* if no alias is used, then I have to entertain the possibility that the user
				 * used a fqfield too so I can know the col_type in bothL
				 *
				 * 	- SELECT f1 FROM V_D_1;
				 * 	- SELECT V_D_1.f1 FROM V_D_1;
				 */

				snprintf(key, sizeof(key) - 1, "%s.%s", table->name, col_name);
				if (!hashtable_put(ht, key, strlen(key) + 1, &col_type, sizeof(col_type))) {
					snprintf(out_err, out_err_len, "duplicate column name: '%s'\n", key);
					return false;
				}

			}

			/* add exprval (name) in its purest form */
			memzero(key, sizeof(key));
			strcpy(key, col_name);

			if (!hashtable_put(ht, key, strlen(key) + 1, &col_type, sizeof(col_type))) {
				snprintf(out_err, out_err_len, "duplicate column name: '%s'\n", key);
				return false;
			}

		}

	} else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!build_column_value_types_ht(db, tmp_entry, node, ht, out_err, out_err_len))
				return false;
		}
	}

	return true;
}

static enum COLUMN_TYPE extract_alias_types(struct ast_node *node, struct ast_sel_alias_node *alias_node, struct hashtable *ht, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_exprval_node *val_node;
	struct ast_sel_fieldname_node *field_node;
	struct hashtable_value *ht_value;
	char lookup_key[FQFIELD_NAME_LEN] = {0};
	enum COLUMN_TYPE col_type = 0;

	if (node->node_type == AST_TYPE_SEL_EXPRVAL) {
		val_node = (typeof(val_node))node;

		if (val_node->value_type.is_intnum) {
			col_type = CT_INTEGER;
		} else if (val_node->value_type.is_approxnum) {
			col_type = CT_DOUBLE;
		} else if (val_node->value_type.is_bool) {
			col_type = CT_TINYINT;
		} else if (val_node->value_type.is_str) {
			col_type = CT_VARCHAR;
		} else if (val_node->value_type.is_name) {
			ht_value = hashtable_get(ht, val_node->name_val, strlen(val_node->name_val) + 1);
			BUG_ON(!ht_value);

			col_type = *(enum COLUMN_TYPE*)ht_value->content;
		} else {
			// most likely a new column type has been added
			BUG_GENERIC();
		}

	} else if (node->node_type == AST_TYPE_SEL_EXPROP) {
		/* recursive expression such as SELECT f1 * 2 as val FROM a; */
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			col_type = extract_alias_types(tmp_entry, alias_node, ht, out_err, out_err_len);
			break; /* single instance is fine as they are supposed to match type-wise */
		}

	} else if (node->node_type == AST_TYPE_SEL_COUNT) {
		/* COUNT function such as SELECT COUNT(f1) as val FROM a; */
		col_type = CT_INTEGER;
	} else if (node->node_type == AST_TYPE_SEL_FIELDNAME) {
		field_node = (typeof(field_node))node;

		snprintf(lookup_key, sizeof(lookup_key) - 1, "%s.%s",
				field_node->table_name,
				field_node->col_name);

		ht_value = hashtable_get(ht, lookup_key, strlen(lookup_key) + 1);
		BUG_ON(!ht_value);

		col_type = *(enum COLUMN_TYPE*)ht_value->content;

	} else {
		/* is there an edge case that I haven't foreseen */
		BUG_GENERIC();
	}

	return col_type;

}

static bool build_alias_value_types_ht(struct ast_node *node, struct hashtable *ht, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_alias_node *alias_node;
	char key[FQFIELD_NAME_LEN] = {0};
	enum COLUMN_TYPE col_type;

	if (node->node_type == AST_TYPE_SEL_ALIAS) {
		alias_node = (typeof(alias_node))node;

		list_for_each(pos, alias_node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (tmp_entry->node_type == AST_TYPE_SEL_TABLE) {
				/* table nodes are not used in this validation, so just move on */
				continue;
			}

			strcpy(key, alias_node->alias_value);
			col_type = extract_alias_types(tmp_entry, alias_node, ht, out_err, out_err_len);

			if (!hashtable_put(ht, key, strlen(key) + 1, &col_type, sizeof(col_type))) {
				snprintf(out_err, out_err_len, "semantic phase: internal error\n");
				return false;
			}

		}
	} else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!build_alias_value_types_ht(tmp_entry, ht, out_err, out_err_len))
				return false;
		}
	}

	return true;
}

static bool check_value_types(struct database *db, struct ast_node *root, char *out_err, size_t out_err_len)
{
	bool valid = true;
	struct hashtable ht = {0};

	if (!hashtable_init(&ht, &hashtable_str_compare, &hashtable_str_hash)) {
		snprintf(out_err, out_err_len, "semantic phase: internal error\n");
		valid = false;
		goto err;
	}

	if (!build_column_value_types_ht(db, root, NULL, &ht, out_err, out_err_len)) {
		valid = false;
		goto err_build_ht;
	}

	if (!build_alias_value_types_ht(root, &ht, out_err, out_err_len)) {
		valid = false;
		goto err_build_ht;
	}

	valid = __check_value_types(root, &ht, out_err, out_err_len).invalid == false;

err_build_ht:
	/* clean up */
	hashtable_foreach(&ht, &free_hashmap_entries, NULL);
	hashtable_free(&ht);
err:
	return valid;
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
		goto cleanup_col_ht;
	}

	/* does tables exist? */
	if (!check_table_names(db, node, out_err, out_err_len))
		goto cleanup_tbl_ht;

	/* build table alias hashtable; check for table alias duplicates */
	if (!check_table_alias(node, out_err, out_err_len, &table_alias))
		goto cleanup_tbl_ht;

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
		goto cleanup_tbl_ht;

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
	 * - SELECT count(*) as val FROM I_D_1 HAVING COUNT(*) > 1; - need to check alias on having statements - Done
	 */
	if (!check_column_names(db, node, node, out_err, out_err_len, &table_alias, &column_alias))
		goto cleanup_tbl_ht;

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
	 * 	SELECT COUNT(f1) + 1 FROM A; (invalid)
	 * 	SELECT (COUNT(f1) + 1) * 2 FROM A; (invalid)
	 */
	if (!check_count_function(node, out_err, out_err_len))
		goto cleanup_tbl_ht;

	/* check select to ensure only columns, recursive expressions, COUNT fncs, and aliases are used.
	 * Expr grammar rule is too generic but that seems to be the case for other engines too.
	 * It's preferable to debug this code than to debug bison's parser code instead :) */
	if (!check_select_clause(node, out_err, out_err_len))
		goto cleanup_tbl_ht;

	/* check JOIN statements */
	if (!check_from_clause(db, node, out_err, out_err_len))
		goto cleanup_tbl_ht;

	/* check where clause */
	if (!check_where_clause(node, out_err, out_err_len, &column_alias))
		goto cleanup_tbl_ht;

	/*
	 * check if group-by clause:
	 * - contains only exprval (name) / fieldnames / alias (later one is verified in check_column_names)
	 * - fields are part of the SELECT fields
	 * 	-> This is also a different thing on SQLITE and MySQL..... got decide how I want it to work.
	 */
	if (!check_groupby_clause(node, out_err, out_err_len, &column_alias))
		goto cleanup_tbl_ht;

	/*
	 * check if order-by clause contains only exprval (name) / fieldnames / alias (later one is verified in
	 * check_column_names)
	 */
	if (!check_orderby_clause(node, out_err, out_err_len, &column_alias))
		goto cleanup_tbl_ht;

	/*
	 * check if having-clause contains exprval (name) / fields / aliases (verified in check_column_names)
	 * and count() occurrences only
	 */
	if (!check_having_clause(node, out_err, out_err_len, &column_alias))
		goto cleanup_tbl_ht;

	/* are all values in the "field IN (xxxxx)" raw values? We can't have fields in there */
	if (!check_isxin_entries(node, out_err, out_err_len))
		goto cleanup_tbl_ht;

	/*
	 * check "IS NULL" and "IS NOT NULL" have fields.
	 * Syntax-wise, "NULL IS NULL" and "NULL IS NOT NULL" is valid
	 * Semantically, this is invalid.
	 */
	if (!check_isxnull_entries(node, out_err, out_err_len))
		goto cleanup_tbl_ht;

	/* are value types correct? Ex.: "SELECT * FROM A where age > 'paulo'" shouldn't be allowed */
	if (!(check_value_types(db, node, out_err, out_err_len)))
		goto cleanup_tbl_ht;

	/* cleanup */
	hashtable_foreach(&column_alias, &free_hashmap_entries, NULL);
	hashtable_free(&column_alias);

	hashtable_foreach(&table_alias, &free_hashmap_entries, NULL);
	hashtable_free(&table_alias);

	return true;

cleanup_tbl_ht:
	hashtable_foreach(&column_alias, &free_hashmap_entries, NULL);
	hashtable_free(&column_alias);

cleanup_col_ht:
	hashtable_foreach(&table_alias, &free_hashmap_entries, NULL);
	hashtable_free(&table_alias);
err:
	return false;
}
