/*
 * semantic_delete.c
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
	if (node->node_type == AST_TYPE_DEL_EXPRVAL) {
		val_node = (typeof(val_node))node;

		if (val_node->value_type.is_name) {
			return column_exists(db, root, val_node);
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

static bool check_isxin_entries(struct ast_node *root, char *out_err, size_t out_err_len)
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

			val_node = (typeof(val_node))tmp_entry;
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
			if (!check_isxin_entries(tmp_entry, out_err, out_err_len))
				return false;
		}
	}

	return true;
}

static bool check_isxnull_entries(struct ast_node *root, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_del_isxnull_node *isxnull_node;
	struct ast_del_exprval_node *val_node;
	struct ast_node *tmp_entry;

	// base case
	if (root->node_type == AST_TYPE_DEL_EXPRISXNULL) {
		isxnull_node = (typeof(isxnull_node))root;

		list_for_each(pos, root->node_children_head)
		{
			val_node = list_entry(pos, typeof(*val_node), head);

			if (!val_node->value_type.is_name) {
				snprintf(out_err, out_err_len, "only fields are allowed in IS NULL|IS NOT NULL\n");
				return false;
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

static bool check_values_field_to_field(struct ast_del_cmp_node *cmp, struct ast_del_exprval_node *val_1,
		struct ast_del_exprval_node *val_2, struct hashtable *ht, char *out_err, size_t out_err_len)
{
	struct hashtable_value *ht_value;
	enum COLUMN_TYPE *type_1, *type_2;

	ht_value = hashtable_get(ht, val_1->name_val, strlen(val_1->name_val) + 1);
	type_1 = (typeof(type_1))ht_value->content;

	ht_value = hashtable_get(ht, val_2->name_val, strlen(val_2->name_val) + 1);
	type_2 = (typeof(type_2))ht_value->content;

	if (*type_1 != *type_2) {
		snprintf(out_err, out_err_len, "field: '%s' and field '%s' don't have the same type\n",
				val_1->name_val,
				val_2->name_val);
		return false;
	} else if (*type_1 == CT_VARCHAR || *type_2 == CT_VARCHAR) {
		/* VARCHAR should only be compared using = or <> operators, anything else is wrong */
		if (cmp->cmp_type != AST_CMP_DIFF_OP && cmp->cmp_type != AST_CMP_EQUALS_OP) {
			snprintf(out_err, out_err_len, "VARCHAR fields can only use '=' or '<>' ops\n");
			return false;
		}
	}

	return true;
}

static bool check_values_field_to_value(struct ast_del_cmp_node *cmp, struct ast_del_exprval_node *field_node,
		struct ast_del_exprval_node *value_node, struct hashtable *ht, char *out_err, size_t out_err_len)
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
		} else if (*type == CT_VARCHAR) {
			/* VARCHAR should only be compared using = or <> operators, anything else is wrong */
			if (cmp->cmp_type != AST_CMP_DIFF_OP && cmp->cmp_type != AST_CMP_EQUALS_OP) {
				snprintf(out_err, out_err_len, "VARCHAR fields can only use '=' or '<>' ops\n");
				return false;
			}
		} else {
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
	} else if (value_node->value_type.is_null) {
		/* NULL should only be compared using = or <> operators, anything else is wrong */
		if (cmp->cmp_type != AST_CMP_DIFF_OP && cmp->cmp_type != AST_CMP_EQUALS_OP) {
			snprintf(out_err, out_err_len, "NULL values can only use '=' or '<>' ops\n");
			return false;
		}
	}
	return true;

}

static bool check_values_value_to_value(struct ast_del_cmp_node *cmp, struct ast_del_exprval_node *val_1,
		struct ast_del_exprval_node *val_2, char *out_err, size_t out_err_len)
{
	bool ret;
	bool cmp_cond;

	ret = memcmp(&val_1->value_type, &val_2->value_type, sizeof(val_1->value_type)) == 0;

	if (!ret) {
		snprintf(out_err, out_err_len, "value-to-value comparison don't have the same type\n");
		return false;
	} else {
		cmp_cond = cmp->cmp_type != AST_CMP_DIFF_OP && cmp->cmp_type != AST_CMP_EQUALS_OP;

		/*
		 * if that's a value-to-value then I can only determine if the operation is supported.
		 *
		 * That means that '2020-02-02' > '2020-02-02' is 'invalid' from a semantic point-of-view
		 * because I can't say for sure that this is a date. (unless we proactively try parsing
		 * it).
		 *
		 * Notes to myself:
		 *
		 * 	Apparently SQLITE does that:
		 *
		 * 	sqlite> select '2020-02-02' > '2020-02-02';
		 * 	0
		 * 	sqlite> select '2020-02-02' < '2020-02-02';
		 * 	0
		 * 	sqlite> select '2020-02-03' > '2020-02-02';
		 * 	1
		 * 	sqlite> select '2020-02-13' > '2020-02-02';
		 * 	1
		 *
		 * Maybe I will implement this in the future - right now this isn't too important as
		 * I am not a  huge fan of value-to-value comparisons to begin with - they are just
		 * silly
		 */

		/* VARCHAR should only be compared using = or <> operators, anything else is wrong */
		if ((val_1->value_type.is_str || val_2->value_type.is_str) && cmp_cond) {
			snprintf(out_err, out_err_len,
					"VARCHAR values '%.128s' and '%.128s' can only use '=' or '<>' ops\n",
					val_1->str_val,
					val_2->str_val);
			return false;
		} else if ((val_1->value_type.is_null || val_2->value_type.is_null) && cmp_cond) {
			/* NULL should only be compared using = or <> operators, anything else is wrong */
			snprintf(out_err, out_err_len, "value-to-value NULL comparisons can only use '=' or '<>'\n");
			return false;
		}
	}

	return true;
}

static bool check_values_for_cmp(struct ast_del_cmp_node *root, struct hashtable *ht, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_del_exprval_node *val_1 = NULL;
	struct ast_del_exprval_node *val_2 = NULL;
	struct ast_del_exprval_node *tmp_entry;

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

		/* sanity check */
		BUG_ON(tmp_entry->node_type != AST_TYPE_DEL_EXPRVAL);

		if (!val_1)
			val_1 = tmp_entry;
		else
			val_2 = tmp_entry;
	}

	if (val_1->value_type.is_name && val_2->value_type.is_name) {
		/* field to field comparison */
		return check_values_field_to_field(root, val_1, val_2, ht, out_err, out_err_len);
	} else if (val_1->value_type.is_name && !val_2->value_type.is_name) {
		/* field-to-value comparison */
		return check_values_field_to_value(root, val_1, val_2, ht, out_err, out_err_len);
	} else if (!val_1->value_type.is_name && val_2->value_type.is_name) {
		/* value-to-field comparison */
		return check_values_field_to_value(root, val_2, val_1, ht, out_err, out_err_len);
	} else {
		/* value-to-value comparison */
		return check_values_value_to_value(root, val_1, val_2, out_err, out_err_len);
	}

}

static bool check_values_for_isxin(struct ast_del_isxin_node *isxin_node, struct hashtable *ht, char *out_err, size_t out_err_len)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_del_cmp_node cmp_node = {0};
	struct ast_del_exprval_node *field_node = NULL, *tmpval_node = NULL;

	/* find field first */
	list_for_each(pos, isxin_node->node_children_head)
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
	list_for_each(pos, isxin_node->node_children_head)
	{
		tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

		if (tmp_entry->node_type == AST_TYPE_DEL_EXPRVAL) {
			tmpval_node = (typeof(tmpval_node))tmp_entry;

			if (!tmpval_node->value_type.is_name) {

				if (isxin_node->is_negation) {
					cmp_node.cmp_type = AST_CMP_DIFF_OP;
				} else {
					cmp_node.cmp_type = AST_CMP_EQUALS_OP;
				}

				if (!check_values_field_to_value(&cmp_node, field_node, tmpval_node,
									ht,
									out_err, out_err_len))
					return false; /* fail fast */
			}
		}
	}
	return true;
}

static bool __check_value_types(struct ast_node *root, struct hashtable *ht, char *out_err, size_t out_err_len)
{

	struct ast_node *tmp_entry;
	struct list_head *pos;

	if (root->node_type == AST_TYPE_DEL_CMP) {
		return check_values_for_cmp((struct ast_del_cmp_node*)root, ht, out_err, out_err_len);
	} else if (root->node_type == AST_TYPE_DEL_EXPRISXIN) {
		return check_values_for_isxin((struct ast_del_isxin_node*)root, ht, out_err, out_err_len);
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
	if (!check_isxin_entries((struct ast_node*)deleteone_node, out_err, out_err_len))
		return false;

	/*
	 * check "IS NULL" and "IS NOT NULL" have fields.
	 * Syntax-wise, "NULL IS NULL" and "NULL IS NOT NULL" is valid
	 * Semantically, this is invalid.
	 */
	if (!check_isxnull_entries((struct ast_node*)deleteone_node, out_err, out_err_len))
		return false;

	/* are value types correct? Ex.: "DELETE FROM A where age > 'paulo'" shouldn't be allowed */
	if (!(check_value_types(db, deleteone_node, out_err, out_err_len)))
		return false;

	return true;
}
