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

static bool check_table_name(struct database *db, struct ast_del_deleteone_node *deleteone_node, char *out_err, size_t out_err_len)
{
	if (!table_validate_name(deleteone_node->table_name)) {
		snprintf(out_err, out_err_len, "table name '%s' is invalid\n", deleteone_node->table_name);
		return false;
	}

	if (!database_table_exists(db, deleteone_node->table_name)) {
		snprintf(out_err, out_err_len, "table name '%s' doesn't exist\n", deleteone_node->table_name);
		return false;
	}

	return true;
}

static bool column_exists(struct database *db, struct ast_del_deleteone_node *deleteone_node, struct ast_del_exprval_node *val_node)
{
	struct table *table;

	table = database_table_get(db, deleteone_node->table_name);

	for (int i = 0; i < table->column_count; i++) {
		if (strncmp(table->columns[i].name, val_node->name_val, sizeof(table->columns[i].name)) == 0) {
			return true;
		}
	}

	return false;
}

static bool __check_column_exists(struct database *db, struct ast_del_deleteone_node *root, struct ast_node *node, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_del_exprval_node *val_node;
	struct ast_node *tmp_entry;

	// base case
	if (list_is_empty(node->node_children_head)) {
		if (node->node_type == AST_TYPE_DEL_EXPRVAL) {
			val_node = (typeof(val_node))node;

			if (val_node->value_type.is_name) {
				return column_exists(db, root, val_node);
			}
		}
	} else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);
			if (!__check_column_exists(db, root, tmp_entry, out_err, out_err_len))
				return false;
		}
	}

	return true;
}

static bool check_column_exists(struct database *db, struct ast_del_deleteone_node *root, char *out_err, size_t out_err_len)
{
	return __check_column_exists(db, root, (struct ast_node*)root, out_err, out_err_len);
}

static bool check_isxin_entries(struct database *db, struct ast_node *root, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_del_isxin_node *isxin_node;
	struct ast_del_exprval_node *val_node;
	struct ast_node *tmp_entry;
	int field_count;

	// base case
	if (root->node_type == AST_TYPE_DEL_EXPRISXIN) {
		isxin_node = (typeof(isxin_node))root;

		field_count = 0;
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (tmp_entry->node_type != AST_TYPE_DEL_EXPRVAL) {
				snprintf(out_err, out_err_len, "IN-clause can only contain raw values\n");
				return false;
			}

			val_node = list_entry(pos, typeof(*val_node), head);
			if (val_node->value_type.is_name) {
				field_count++;
			}

			/*
			 * ast_del_isxin_node holds the field which is part of the expression in the
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
			if (!check_isxin_entries(db, tmp_entry, out_err, out_err_len))
				return false;
		}
	}

	return true;
}

static bool check_null_cmp(struct database *db, struct ast_node *root, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_del_cmp_node *cmp_node;
	struct ast_del_exprval_node *val_node;
	struct ast_node *tmp_entry;

	list_for_each(pos, root->node_children_head)
	{
		tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

		// base case
		if (list_is_empty(tmp_entry->node_children_head)) {
			if (root->node_type == AST_TYPE_DEL_CMP && tmp_entry->node_type == AST_TYPE_DEL_EXPRVAL) {
				cmp_node = (typeof(cmp_node))root;
				val_node = (typeof(val_node))tmp_entry;

				if (val_node->value_type.is_null && (cmp_node->cmp_type != AST_CMP_DIFF_OP
						&& cmp_node->cmp_type != AST_CMP_EQUALS_OP)) {
					snprintf(out_err,
							out_err_len,
							"NULL fields can only be compared using '=' or '<>' operators\n");
					return false;
				}

			}
		} else {
			if (!check_null_cmp(db, tmp_entry, out_err, out_err_len))
				return false;
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

static bool check_value_for_field(struct ast_del_exprval_node *field_node, struct ast_del_exprval_node *value_node,
		struct hashtable *ht, char *out_err, size_t out_err_len)
{
	struct hashtable_value *ht_value;
	enum COLUMN_TYPE *type;

	ht_value = hashtable_get(ht, field_node->name_val, strlen(field_node->name_val) + 1);
	type = (typeof(type))ht_value->content;

	if (value_node->value_type.is_str) {
		if (*type == CT_DATE || *type == CT_DATETIME) {
			if (!try_parse_date_type(value_node->str_val, *type)) {
				snprintf(out_err, out_err_len,
						/* max str size would exceed buffer's size, so trim it */
						"val: '%.256s' can't be parsed for DATE | DATETIME column\n",
						value_node->str_val);
				return false;
			}
		} else if (*type != CT_VARCHAR) {
			snprintf(out_err, out_err_len,
					/* max str size would exceed buffer's size, so trim it */
					"val: '%.256s' requires an VARCHAR() column\n",
					value_node->str_val);
			return false;
		}
	} else if (value_node->value_type.is_intnum && *type != CT_INTEGER) {
		snprintf(out_err, out_err_len, "val: '%ld' requires an INTEGER column\n", value_node->int_val);
		return false;
	} else if (value_node->value_type.is_approxnum && *type != CT_DOUBLE) {
		snprintf(out_err, out_err_len, "val: '%f' requires a DOUBLE column\n", value_node->double_val);
		return false;
	} else if (value_node->value_type.is_bool && *type != CT_TINYINT) {
		snprintf(out_err, out_err_len, "val: '%d' requires a TINYINT column\n", value_node->bool_val);
		return false;
	}

	return true;

}

static bool __check_value_types(struct ast_node *root, struct hashtable *ht, char *out_err, size_t out_err_len)
{
	struct ast_del_exprval_node *field_node = NULL, *value_node = NULL, *tmpval_node = NULL;
	struct ast_node *tmp_entry;
	struct list_head *pos;

	if (list_is_empty(root->node_children_head)) {
		// base case -> we can't go any further
		return true;
	} else {
		if (root->node_type == AST_TYPE_DEL_CMP) {
			/*
			 * while not guaranteed, CMP node will generally have [FIELD, VAL] as children.
			 * This is because "field1 CMP field2" or "value1 CMP value2" are technically valid..
			 * Although, I must say that "value1 CMP value2" is stupid and also potentially the biggest
			 * culprit when it comes to most SQL injections cases in the world...
			 *
			 * I wonder if I should forbid "value1 CMP value2" in the semantic phase??
			 */

			list_for_each(pos, root->node_children_head)
			{
				tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

				if (tmp_entry->node_type == AST_TYPE_DEL_EXPRVAL) {
					tmpval_node = (typeof(tmpval_node))tmp_entry;

					if (tmpval_node->value_type.is_name) {
						field_node = tmpval_node;
					} else {
						value_node = tmpval_node;
					}
				}
			}

			if (field_node && value_node) {
				return check_value_for_field(field_node, value_node, ht, out_err, out_err_len);
			}
		} else if (root->node_type == AST_TYPE_DEL_EXPRISXIN) {

			/* find field first */
			list_for_each(pos, root->node_children_head)
			{
				tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

				if (tmp_entry->node_type == AST_TYPE_DEL_EXPRVAL) {
					tmpval_node = (typeof(tmpval_node))tmp_entry;

					if (tmpval_node->value_type.is_name) {
						field_node = tmpval_node;
						break;
					}
				}
			}

			/* go through values now*/
			list_for_each(pos, root->node_children_head)
			{
				tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

				if (tmp_entry->node_type == AST_TYPE_DEL_EXPRVAL) {
					tmpval_node = (typeof(tmpval_node))tmp_entry;

					if (!tmpval_node->value_type.is_name) {
						if (!check_value_for_field(field_node,
										tmpval_node,
										ht, out_err,
										out_err_len))
							return false; /* fail fast */
					}
				}
			}

		} else {
			// traverse the tree
			list_for_each(pos, root->node_children_head)
			{
				tmp_entry = list_entry(pos, typeof(*tmp_entry), head);
				if (!__check_value_types(tmp_entry, ht, out_err, out_err_len))
					return false;
			}
		}
		return true;
	}
}

static bool check_value_types(struct database *db, struct ast_del_deleteone_node *root, char *out_err, size_t out_err_len)
{
	bool valid = true;
	struct hashtable ht = {0};
	struct table *table;

	if (!hashtable_init(&ht, &hashtable_str_compare, &hashtable_str_hash)) {
		snprintf(out_err, out_err_len, "semantic phase: internal error\n");
		valid = false;
		goto err;
	}

	table = database_table_get(db, root->table_name);

	for (int i = 0; i < table->column_count; i++) {
		char *key = table->columns[i].name;
		enum COLUMN_TYPE value = table->columns[i].type;

		if (!hashtable_put(&ht, key, strlen(key) + 1, &value, sizeof(value))) {
			snprintf(out_err, out_err_len, "semantic phase: internal error\n");
			valid = false;
			goto err_ht_put_col;
		}
	}

	valid = __check_value_types((struct ast_node*)root, &ht, out_err, out_err_len);

err_ht_put_col:
	/* clean up */
	hashtable_foreach(&ht, &free_hashmap_entries, NULL);
	hashtable_free(&ht);
err:
	return valid;
}

bool semantic_analyse_delete_stmt(struct database *db, struct ast_node *node, char *out_err, size_t out_err_len)
{
	struct ast_del_deleteone_node *deleteone_node;

	deleteone_node = (typeof(deleteone_node))node;

	/* does table exist? */
	if (!check_table_name(db, deleteone_node, out_err, out_err_len))
		return false;

	/* do all columns specified in the statement exist in the table? */
	if (!check_column_exists(db, deleteone_node, out_err, out_err_len))
		return false;

	/* are all values in the "field IN (xxxxx)" raw values? We can't have fields in there */
	if (!check_isxin_entries(db, (struct ast_node*)deleteone_node, out_err, out_err_len))
		return false;

	/* NULL should only be compared using = or <> operators, anything else is wrong */
	if (!check_null_cmp(db, (struct ast_node*)deleteone_node, out_err, out_err_len))
		return false;

	/* are value types correct? Ex.: "DELETE FROM A where age > 'paulo'" shouldn't be allowed */
	if (!(check_value_types(db, deleteone_node, out_err, out_err_len)))
		return false;

	return true;
}
