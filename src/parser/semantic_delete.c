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

static bool __check_column_exists(struct database *db, struct ast_del_deleteone_node *root,struct ast_node *node, char *out_err, size_t out_err_len)
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

bool semantic_analyse_delete_stmt(struct database *db, struct ast_node *node, char *out_err, size_t out_err_len)
{
	struct ast_del_deleteone_node *deleteone_node;

	deleteone_node = (typeof(deleteone_node))node;

	/* does table exist? */
	if (!check_table_name(db, deleteone_node, out_err, out_err_len))
		return false;

	/* do all terms (columns) specified in the statement exist in the table? */
	if (!check_column_exists(db, deleteone_node, out_err, out_err_len))
		return false;


	return true;
}
