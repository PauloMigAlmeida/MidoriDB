/*
 * executor_delete.c
 *
 *  Created on: 27/06/2023
 *	Author: paulo
 */

#define _XOPEN_SOURCE       /* See feature_test_macros(7) */
#include <engine/executor.h>
#include <primitive/table.h>
#include <primitive/column.h>
#include <primitive/row.h>
#include <datastructure/linkedlist.h>

static time_t parse_date_type(char *str, enum COLUMN_TYPE type)
{
	time_t time_out;
	struct tm time_struct = {0};
	const char *fmt;

	if (type == CT_DATE)
		fmt = COLUMN_CTDATE_FMT;
	else
		fmt = COLUMN_CTDATETIME_FMT;

	/* semantic phase should guarantee that this won't ever fail */
	strptime(str, fmt, &time_struct);

	time_out = mktime(&time_struct);

	return time_out;

}

static bool cmp_double_value_to_value(enum ast_comparison_type cmp_type, double val_1, double val_2)
{
	switch (cmp_type) {
	case AST_CMP_DIFF_OP:
		return val_1 != val_2;
	case AST_CMP_EQUALS_OP:
		return val_1 == val_2;
	case AST_CMP_GTE_OP:
		return val_1 >= val_2;
	case AST_CMP_GT_OP:
		return val_1 > val_2;
	case AST_CMP_LTE_OP:
		return val_1 <= val_2;
	case AST_CMP_LT_OP:
		return val_1 < val_2;
	default:
		/* something went really wrong here */
		BUG_GENERIC();
		return false;
	}
}

static bool cmp_int_value_to_value(enum ast_comparison_type cmp_type, int val_1, int val_2)
{
	switch (cmp_type) {
	case AST_CMP_DIFF_OP:
		return val_1 != val_2;
	case AST_CMP_EQUALS_OP:
		return val_1 == val_2;
	case AST_CMP_GTE_OP:
		return val_1 >= val_2;
	case AST_CMP_GT_OP:
		return val_1 > val_2;
	case AST_CMP_LTE_OP:
		return val_1 <= val_2;
	case AST_CMP_LT_OP:
		return val_1 < val_2;
	default:
		/* something went really wrong here */
		BUG_GENERIC();
		return false;
	}
}

static bool cmp_bool_value_to_value(enum ast_comparison_type cmp_type, bool val_1, bool val_2)
{
	switch (cmp_type) {
	case AST_CMP_DIFF_OP:
		return val_1 != val_2;
	case AST_CMP_EQUALS_OP:
		return val_1 == val_2;
	default:
		return false;
	}
}

static bool cmp_str_value_to_value(enum ast_comparison_type cmp_type, char *val_1, char *val_2)
{
	switch (cmp_type) {
	case AST_CMP_DIFF_OP:
		return strcmp(val_1, val_2);
	case AST_CMP_EQUALS_OP:
		return strcmp(val_1, val_2) == 0;
	default:
		return false;
	}
}

static bool cmp_time_value_to_value(enum ast_comparison_type cmp_type, time_t val_1, time_t val_2)
{
	switch (cmp_type) {
	case AST_CMP_DIFF_OP:
		return val_1 != val_2;
	case AST_CMP_EQUALS_OP:
		return val_1 == val_2;
	case AST_CMP_GTE_OP:
		return val_1 >= val_2;
	case AST_CMP_GT_OP:
		return val_1 > val_2;
	case AST_CMP_LTE_OP:
		return val_1 <= val_2;
	case AST_CMP_LT_OP:
		return val_1 < val_2;
	default:
		/* something went really wrong here */
		BUG_GENERIC();
		return false;
	}
}

static bool cmp_field_to_field(struct table *table, struct row *row, struct ast_del_cmp_node *node, struct ast_del_exprval_node *val_1, struct ast_del_exprval_node *val_2)
{
	enum COLUMN_TYPE type = 0;
	size_t tmp_offset = 0, offset_1 = 0, offset_2 = 0;
	int col_idx_1 = -1, col_idx_2 = -1;
	bool is_null_1, is_null_2;

	for (int i = 0; i < table->column_count; i++) {
		struct column *col = &table->columns[i];

		if (strcmp(col->name, val_1->name_val) == 0) {
			/* semantic phase guarantees that columns will have the same type in CMP nodes */
			type = col->type;
			offset_1 = tmp_offset;
			col_idx_1 = i;
		} else if (strcmp(col->name, val_2->name_val) == 0) {
			offset_2 = tmp_offset;
			col_idx_2 = i;
		}
		tmp_offset += table_calc_column_space(col);
	}

	/* is any field set to null in this row ? */
	is_null_1 = bit_test(row->null_bitmap, col_idx_1, sizeof(row->null_bitmap));
	is_null_2 = bit_test(row->null_bitmap, col_idx_2, sizeof(row->null_bitmap));

	if (is_null_1 || is_null_2) {
		/*
		 * if even one field is NULL, then no comparison would evaluate to true.
		 *
		 * That one was a bit odd as I felt tempted to use a truth table, then again if 2 fields where NULL,
		 * they ought to return True if I compare field1 = field2, right?
		 *
		 * Wrong! (let that sink in for a moment.............. I know, right?)
		 *
		 * I noticed that SQLITE implements the following:
		 *
		 * sqlite> create table a(f1 int, f2 int);
		 * sqlite> insert into a values (1,1), (1, NULL), (NULL,1), (NULL,NULL);
		 * sqlite> select * from a where f1 <> f2;
		 * 	<nothing...> - baffling, right?
		 * sqlite> select * from a where f1 == f2;
		 * f1  f2
		 * --  --
		 * 1   1
		 * 	<I would expect to see NULL, NULL here... but this isn't how it works >
		 * sqlite>
		 */
		return false;
	} else if (type == CT_DOUBLE) {
		return cmp_double_value_to_value(node->cmp_type,
							*(double*)&row->data[offset_1],
							*(double*)&row->data[offset_2]);
	} else if (type == CT_TINYINT) {
		return cmp_bool_value_to_value(node->cmp_type,
						*(bool*)&row->data[offset_1],
						*(bool*)&row->data[offset_2]);
	} else if (type == CT_INTEGER) {
		return cmp_int_value_to_value(node->cmp_type,
						*(int64_t*)&row->data[offset_1],
						*(int64_t*)&row->data[offset_2]);
	} else if (type == CT_DATE || type == CT_DATETIME) {
		return cmp_time_value_to_value(node->cmp_type,
						*(time_t*)&row->data[offset_1],
						*(time_t*)&row->data[offset_2]);
	} else if (type == CT_VARCHAR) {
		return cmp_str_value_to_value(node->cmp_type,
						*(char**)&row->data[offset_1],
						*(char**)&row->data[offset_2]);
	} else {
		/* something went really wrong here */
		BUG_GENERIC();
		return false;
	}

}

static bool cmp_field_to_value(struct table *table, struct row *row, struct ast_del_cmp_node *node,
		struct ast_del_exprval_node *field, struct ast_del_exprval_node *value)
{
	enum COLUMN_TYPE type = 0;
	size_t offset = 0;
	int col_idx = -1;
	bool is_null;

	for (int i = 0; i < table->column_count; i++) {
		struct column *col = &table->columns[i];

		if (strcmp(col->name, field->name_val) == 0) {
			type = col->type;
			col_idx = i;
			break;
		}
		offset += table_calc_column_space(col);
	}

	is_null = bit_test(row->null_bitmap, col_idx, sizeof(row->null_bitmap));

	if (is_null || value->value_type.is_null) {
		/* no comparison evaluates to true if NULL is one of the operands */
		return false;
	} else if (type == CT_DOUBLE) {
		return cmp_double_value_to_value(node->cmp_type, *(double*)&row->data[offset], value->double_val);
	} else if (type == CT_TINYINT) {
		return cmp_bool_value_to_value(node->cmp_type, *(bool*)&row->data[offset], value->bool_val);
	} else if (type == CT_INTEGER) {
		return cmp_int_value_to_value(node->cmp_type, *(int64_t*)&row->data[offset], value->int_val);
	} else if (type == CT_DATE || type == CT_DATETIME) {
		return cmp_time_value_to_value(node->cmp_type,
						*(time_t*)&row->data[offset],
						parse_date_type(value->str_val, type));
	} else if (type == CT_VARCHAR) {
		return cmp_str_value_to_value(node->cmp_type, *(char**)&row->data[offset], value->str_val);
	} else {
		/* something went really wrong here */
		BUG_GENERIC();
		return false;
	}

}

static bool cmp_value_to_value(struct ast_del_cmp_node *node, struct ast_del_exprval_node *val_1, struct ast_del_exprval_node *val_2)
{
	if (val_1->value_type.is_approxnum) {
		return cmp_double_value_to_value(node->cmp_type, val_1->double_val, val_2->double_val);
	} else if (val_1->value_type.is_bool) {
		return cmp_bool_value_to_value(node->cmp_type, val_1->bool_val, val_2->bool_val);
	} else if (val_1->value_type.is_intnum) {
		return cmp_int_value_to_value(node->cmp_type, val_1->int_val, val_2->int_val);
	} else if (val_1->value_type.is_null) {
		/* no comparison evaluates to true if NULL is one of the operands */
		return false;
	} else if (val_1->value_type.is_str) {
		return cmp_str_value_to_value(node->cmp_type, val_1->str_val, val_2->str_val);
	} else {
		/* something went really wrong here */
		BUG_GENERIC();
		return false;
	}

}

static bool eval_cmp(struct table *table, struct row *row, struct ast_del_cmp_node *node)
{
	struct list_head *pos;
	struct ast_node *tmp_node;
	struct ast_del_exprval_node *val_1 = NULL, *val_2 = NULL;

	list_for_each(pos, node->node_children_head)
	{
		tmp_node = list_entry(pos, typeof(*tmp_node), head);

		if (!val_1)
			val_1 = (typeof(val_1))tmp_node;
		else
			val_2 = (typeof(val_2))tmp_node;
	}

	if (val_1->value_type.is_name && val_2->value_type.is_name) {
		/* field to field comparison */
		return cmp_field_to_field(table, row, node, val_1, val_2);
	} else if (val_1->value_type.is_name && !val_2->value_type.is_name) {
		/* field-to-value or value-to-field comparison */
		return cmp_field_to_value(table, row, node, val_1, val_2);
	} else if (!val_1->value_type.is_name && val_2->value_type.is_name) {
		/* value-to-field or value-to-field comparison */
		return cmp_field_to_value(table, row, node, val_2, val_1);
	} else {
		/* value-to-value comparison */
		return cmp_value_to_value(node, val_1, val_2);
	}

}

static bool eval_isxnull(struct table *table, struct row *row, struct ast_del_isxnull_node *node)
{
	struct list_head *pos;
	struct ast_del_exprval_node *field = NULL;
	int col_idx = -1;

	list_for_each(pos, node->node_children_head)
	{
		field = list_entry(pos, typeof(*field), head);
	}

	BUG_ON(!field);

	for (int i = 0; i < table->column_count; i++) {
		struct column *col = &table->columns[i];

		if (strcmp(col->name, field->name_val) == 0) {
			col_idx = i;
			break;
		}
	}

	return bit_test(row->null_bitmap, col_idx, sizeof(row->null_bitmap)) ^ node->is_negation;
}

static bool eval_isxin(struct table *table, struct row *row, struct ast_del_isxin_node *node)
{
	struct list_head *pos;
	struct ast_del_exprval_node *tmp_entry;
	struct ast_del_exprval_node *field = NULL;
	struct ast_del_cmp_node cmp_node = {0};

	if (node->is_negation) {
		cmp_node.cmp_type = AST_CMP_DIFF_OP;
	} else {
		cmp_node.cmp_type = AST_CMP_EQUALS_OP;
	}

	list_for_each(pos, node->node_children_head)
	{
		tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

		if (tmp_entry->value_type.is_name) {
			field = tmp_entry;
			break;
		}
	}

	BUG_ON(!field);

	list_for_each(pos, node->node_children_head)
	{
		tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

		if (!tmp_entry->value_type.is_name) {
			if (cmp_field_to_value(table, row, &cmp_node, field, tmp_entry))
				return true;
		}
	}
	return false;
}

static bool eval_delete_row(struct table *table, struct row *row, struct ast_node *node)
{
	struct list_head *pos;
	struct ast_node *tmp_node;
	struct ast_del_logop_node *logop_node;
	bool ret = true;

	if (node->node_type == AST_TYPE_DEL_CMP) {
		return eval_cmp(table, row, (struct ast_del_cmp_node*)node);
	} else if (node->node_type == AST_TYPE_DEL_EXPRISXNULL) {
		return eval_isxnull(table, row, (struct ast_del_isxnull_node*)node);
	} else if (node->node_type == AST_TYPE_DEL_EXPRISXIN) {
		return eval_isxin(table, row, (struct ast_del_isxin_node*)node);
	} else if (node->node_type == AST_TYPE_DEL_DELETEONE) {
		list_for_each(pos, node->node_children_head)
		{
			tmp_node = list_entry(pos, typeof(*tmp_node), head);
			ret = ret && eval_delete_row(table, row, tmp_node);
		}

	} else {
		/* sanity check */
		BUG_ON(node->node_type != AST_TYPE_DEL_LOGOP);

		logop_node = (typeof(logop_node))node;

		bool gate_init = false;

		list_for_each(pos, logop_node->node_children_head)
		{
			tmp_node = list_entry(pos, typeof(*tmp_node), head);

			bool eval = eval_delete_row(table, row, tmp_node);

			if (!gate_init) {
				gate_init = true;
				ret = eval;
			} else if (logop_node->logop_type == AST_LOGOP_TYPE_AND)
				ret = ret && eval;
			else if (logop_node->logop_type == AST_LOGOP_TYPE_OR)
				ret = ret || eval;
			else if (logop_node->logop_type == AST_LOGOP_TYPE_XOR)
				ret = ret ^ eval;
			else
				BUG_GENERIC();
		}

	}
	return ret;
}

static int scan_delete(struct table *table, struct ast_node *node, struct query_output *output)
{
	struct list_head *pos;
	struct datablock *block;
	struct row *row;
	size_t row_size;

	row_size = table_calc_row_size(table);

	list_for_each(pos, table->datablock_head)
	{
		block = list_entry(pos, typeof(*block), head);

		for (size_t i = 0; i < (DATABLOCK_PAGE_SIZE / row_size); i++) {
			row = (typeof(row))&block->data[row_size * i];

			if (!row->flags.deleted && !row->flags.empty && eval_delete_row(table, row, node)) {

				if (!table_delete_row(table, block, i * row_size)) {
					return -MIDORIDB_INTERNAL;
				}

				output->n_rows_aff++;
			}

		}
	}

	return MIDORIDB_OK;
}

int executor_run_deleteone_stmt(struct database *db, struct ast_del_deleteone_node *delete_node, struct query_output *output)
{
	struct table *table;
	int rc = MIDORIDB_OK;

	if (!database_table_exists(db, delete_node->table_name)) {
		rc = -MIDORIDB_INTERNAL;
		goto out;
	}

	table = database_table_get(db, delete_node->table_name);

	rc = scan_delete(table, (struct ast_node*)delete_node, output);

out:
	return rc;
}
