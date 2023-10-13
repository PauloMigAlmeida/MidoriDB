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
		bool is_expre; /* Ex.: select f1 * 2 as val from A*/
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
				BUG_ON(true);
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

static bool check_column_names_select(struct database *db, struct ast_node *root, struct ast_node *node, char *out_err, size_t out_err_len)
{

	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_exprval_node *val_node;
	int count = 0;

	/* base case 1 */
	if (node->node_type == AST_TYPE_SEL_WHERE || node->node_type == AST_TYPE_SEL_JOIN
			|| node->node_type == AST_TYPE_SEL_HAVING || node->node_type == AST_TYPE_SEL_GROUPBY
			|| node->node_type == AST_TYPE_SEL_ORDERBYLIST
			|| node->node_type == AST_TYPE_SEL_LIMIT) {
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
	/* recursion  */
	else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_column_names_select(db, root, tmp_entry, out_err, out_err_len))
				return false;
		}
	}

	return true;
}

static bool check_column_names_groupby(struct database *db, struct ast_node *root, struct ast_node *node, char *out_err,
		size_t out_err_len, struct hashtable *column_alias)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_exprval_node *valn;
	int count = 0;

	if (node->node_type == AST_TYPE_SEL_GROUPBY) {
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
			}
		}

	} else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_column_names_groupby(db, root, tmp_entry, out_err, out_err_len, column_alias))
				return false;
		}
	}

	return true;
}

static bool check_column_names_orderby(struct database *db, struct ast_node *root, struct ast_node *node, char *out_err,
		size_t out_err_len, struct hashtable *column_alias)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_exprval_node *valn;
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
			}
		}

	} else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_column_names_orderby(db, root, tmp_entry, out_err, out_err_len, column_alias))
				return false;
		}
	}

	return true;
}

static bool check_column_names(struct database *db, struct ast_node *root, struct ast_node *node, char *out_err,
		size_t out_err_len, struct hashtable *column_alias)
{
	/*
	 * check if EXPRVAL (names) point to something that does not exist or that is ambiguous on SELECT fields.
	 * this handles cases like:
	 *
	 * 	CREATE TABLE A (f1 INT);
	 * 	CREATE TABLE B (f1 INT);
	 *
	 * 	SELECT f2 FROM A; (column doesn't exist)
	 * 	SELECT f1 FROM A, B; (column exists on both A and B so we can't tell from which to fetch data from)
	 * 	SELECT f1 as val, val * 2 as x FROM A; (alias can't be used as the source for yet another alias)
	 */
	if (!check_column_names_select(db, root, node, out_err, out_err_len))
		return false;
	/*
	 * check if EXPRVAL (names) in the group by clause are either columns or alias... Also ensure that none of them
	 * are ambiguous if more than 1 table is used. This should handle cases like:
	 *
	 * 	CREATE TABLE A (f1 INT);
	 * 	CREATE TABLE B (f1 INT);
	 *
	 * 	SELECT f1 FROM A GROUP BY f1; (okay)
	 * 	SELECT f1 as val FROM A GROUP BY val; (okay)
	 * 	SELECT f1 FROM A, B GROUP BY f1; (f1 is ambiguous)
	 * 	SELECT f1 FROM A GROUP BY f2; (no such column)
	 */
	if (!check_column_names_groupby(db, root, node, out_err, out_err_len, column_alias))
		return false;

	/*
	 * check if EXPRVAL (names) in the order by clause are either columns or alias... Also ensure that none of them
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
	if (!check_column_names_orderby(db, root, node, out_err, out_err_len, column_alias))
		return false;

	return true;
}

static bool check_groupby_clause_expr(struct database *db, struct ast_node *root, struct ast_node *node, char *out_err,
		size_t out_err_len, struct hashtable *column_alias)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_exprval_node *valn;

	if (node->node_type == AST_TYPE_SEL_GROUPBY) {
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
				snprintf(out_err, out_err_len, "group-by clauses support only fields and aliases\n");
				return false;
			}
		}

	} else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_groupby_clause_expr(db, root, tmp_entry, out_err, out_err_len, column_alias))
				return false;
		}
	}

	return true;

}

static bool check_groupby_clause(struct database *db, struct ast_node *root, struct ast_node *node, char *out_err,
		size_t out_err_len, struct hashtable *column_alias)
{
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
	if (!check_groupby_clause_expr(db, root, node, out_err, out_err_len, column_alias))
		return false;

	return true;
}

static bool check_orderby_clause_expr(struct database *db, struct ast_node *root, struct ast_node *node, char *out_err,
		size_t out_err_len, struct hashtable *column_alias)
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
				snprintf(out_err, out_err_len, "order-by clauses support only fields and aliases\n");
				return false;
			}
		}

	} else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (!check_orderby_clause_expr(db, root, tmp_entry, out_err, out_err_len, column_alias))
				return false;
		}
	}

	return true;

}

static bool check_orderby_clause(struct database *db, struct ast_node *root, struct ast_node *node, char *out_err,
		size_t out_err_len, struct hashtable *column_alias)
{
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
	if (!check_orderby_clause_expr(db, root, node, out_err, out_err_len, column_alias))
		return false;

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
	 * check columns - simple columns
	 *
	 * Column name (Simple) challenges
	 *
	 * CREATE TABLE I_D_1 (f1 int);
	 * CREATE TABLE I_D_2 (f2 int);
	 *
	 * - SELECT f2 FROM I_D_1 JOIN I_D_2 ON f1 = f3; - f3 clearly don't exist in I_D_1 or I_D_2... so all good here
	 * - SELECT f2 FROM I_D_1 JOIN I_D_2 ON f1 = f1; - I need a join checker as I can't validate if I'm pointing to
	 * 	fields on both sides of the join.... this can get tricky when accounting for multiple joins
	 * - SELECT f1 as val FROM I_D_1 WHERE val > 2; - I can still choose if that's right or not.
	 * - SELECT f1 as val FROM I_D_1 GROUP BY val; - need to check alias on group by statements
	 * - SELECT f1 as val FROM I_D_1 ORDER BY val; - need to check alias on order by statements.
	 * - SELECT count(*) as val FROM I_D_1 HAVING COUNT(*) > 1; - need to check alias on having statements
	 * 	TODO: Figure this out
	 */
	if (!check_column_names(db, node, node, out_err, out_err_len, &column_alias))
		goto err_column_names;

	/* TODO check columns - fieldname columns */

	/*
	 * check if group-by clause:
	 * - contains only exprval (name) / fieldnames / alias (later one is verified in check_column_names)
	 * - fields are part of the SELECT fields
	 *
	 */
	if (!check_groupby_clause(db, node, node, out_err, out_err_len, &column_alias))
		goto err_groupby_clause;

	/*
	 * check if order-by clause contains only exprval (name) / fieldnames / alias (later one is verified in
	 * check_column_names)
	 */
	if (!check_orderby_clause(db, node, node, out_err, out_err_len, &column_alias))
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
