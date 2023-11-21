/*
 * executor_select.c
 *
 * Notes to myself:
 * 	- I think I will go for the early materialization approach
 * 		(copy data of table(s) into a new table before returning the query)
 *	- As much as I want to implement the Sort-Merge Join algorithm, I know that
 *		implementing the execution plan for SELECT statements will be hard
 *		already. So for now, I will keep things simple and go for the
 *		Naive-nested loop algorithm first.
 *		As a strech-goal, maybe implement Block-nested-loop algorithm
 *	- Alternatively, if I ever implement Indexes, then a lot of the inefficiencies
 *		of the naive-nested loop should go away
 *
 *
 *  Created on: 15/11/2023
 *      Author: paulo
 */

#define _XOPEN_SOURCE       /* See feature_test_macros(7) */
#include <engine/executor.h>
#include <primitive/row.h>

#define FQFIELD_NAME_LEN 	MEMBER_SIZE(struct ast_sel_fieldname_node, table_name)			\
					+ 1 /* dot */ 							\
					+ MEMBER_SIZE(struct ast_sel_fieldname_node, col_name)		\
					+ 1 /* NUL */

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

static int _build_cols_hashtable_fieldname(struct database *db, struct ast_node *node, struct hashtable *cols_ht)
{
	struct ast_sel_fieldname_node *field_node;
	struct table *table;
	struct column *column;
	char key[FQFIELD_NAME_LEN] = {0};

	field_node = (typeof(field_node))node;
	table = database_table_get(db, field_node->table_name);

	for (int i = 0; i < table->column_count; i++) {
		column = &table->columns[i];

		if (strcmp(field_node->col_name, column->name) == 0) {
			snprintf(key, sizeof(key) - 1, "%s.%s", field_node->table_name, column->name);

			if (!hashtable_put(cols_ht, key, strlen(key) + 1, &column, sizeof(column)))
				return -MIDORIDB_INTERNAL;
		}
	}
	return MIDORIDB_OK;
}

static int _build_cols_hastable_alias(struct database *db, struct ast_node *node, struct ast_sel_alias_node *alias_node,
		struct hashtable *cols_ht)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_exprval_node *val_node;
	struct column column = {0};
	int ret = MIDORIDB_OK;

	if (node->node_type == AST_TYPE_SEL_EXPRVAL) {
		val_node = (typeof(val_node))node;

		/* sanity check - there shouldn't be any exprvals (name) at this point in here */
		BUG_ON(val_node->value_type.is_name);

		if (val_node->value_type.is_str) {
			column.type = CT_VARCHAR; // PS: raw values are not auto-boxed into CT_DATE/CT_DATETIMEs
			column.precision = strlen(val_node->str_val) + 1;
		} else {
			if (val_node->value_type.is_approxnum)
				column.type = CT_DOUBLE;
			else if (val_node->value_type.is_intnum)
				column.type = CT_INTEGER;
			else if (val_node->value_type.is_bool)
				column.type = CT_TINYINT;
			else
				BUG_GENERIC();

			column.precision = table_calc_column_precision(column.type);
		}

		if (!hashtable_put(cols_ht, alias_node->alias_value, strlen(alias_node->alias_value) + 1,
					&column,
					sizeof(column)))
			ret = -MIDORIDB_INTERNAL;

	} else if (node->node_type == AST_TYPE_SEL_FIELDNAME) {
		ret = _build_cols_hashtable_fieldname(db, node, cols_ht);
	} else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			return _build_cols_hastable_alias(db, tmp_entry, (struct ast_sel_alias_node*)tmp_entry,
								cols_ht);
		}
	}

	return ret;
}

static int _build_cols_hashtable_table(struct database *db, struct ast_node *node, struct hashtable *cols_ht)
{
	struct ast_sel_table_node *table_node;
	struct table *table;
	struct column *column;
	char key[FQFIELD_NAME_LEN];

	table_node = (typeof(table_node))node;
	table = database_table_get(db, table_node->table_name);

	for (int i = 0; i < table->column_count; i++) {
		column = &table->columns[i];

		memzero(key, sizeof(key));
		snprintf(key, sizeof(key) - 1, "%s.%s", table_node->table_name, column->name);

		if (!hashtable_put(cols_ht, key, strlen(key) + 1, column, sizeof(*column)))
			return -MIDORIDB_INTERNAL;
	}

	return MIDORIDB_OK;
}

static int build_cols_hashtable(struct database *db, struct ast_node *node, struct hashtable *cols_ht)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	int ret = MIDORIDB_OK;

	if (node->node_type == AST_TYPE_SEL_ALIAS) {
		return _build_cols_hastable_alias(db, node, (struct ast_sel_alias_node*)node, cols_ht);
	} else if (node->node_type == AST_TYPE_SEL_TABLE) {
		return _build_cols_hashtable_table(db, node, cols_ht);
	} else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if ((ret = build_cols_hashtable(db, tmp_entry, cols_ht)))
				break;
		}

	}
	return ret;

}

static void do_build_table_scafold(struct hashtable *hashtable, const void *key, size_t klen, const void *value,
		size_t vlen, void *arg)
{
	UNUSED(hashtable);
	UNUSED(vlen);

	struct table *table;
	struct column column = {0};

	table = (typeof(table))arg;

	strncpy(column.name, key, MIN(sizeof(column.name), klen) - 1);
	column.type = ((struct column*)value)->type;
	column.precision = ((struct column*)value)->precision;

	BUG_ON(!table_add_column(table, &column));
}

static int build_table_scafold(struct hashtable *column_types, struct table **out_table)
{
	*out_table = table_init("early_mat_tbl");

	if (!(*out_table))
		return -MIDORIDB_INTERNAL;

	hashtable_foreach(column_types, &do_build_table_scafold, *out_table);

	return MIDORIDB_OK;
}

static int proc_from_clause_table(struct database *db, struct ast_sel_table_node *table_node, struct table *earmattbl)
{
	struct list_head *pos;
	struct table *table;
	struct datablock *block;
	struct row *row;
	size_t row_size;

	table = database_table_get(db, table_node->table_name);

	row_size = table_calc_row_size(table);

	list_for_each(pos, table->datablock_head)
	{
		block = list_entry(pos, typeof(*block), head);

		for (size_t i = 0; i < (DATABLOCK_PAGE_SIZE / row_size); i++) {
			row = (typeof(row))&block->data[row_size * i];

			if (!row->flags.deleted && !row->flags.empty) {
				if (!table_insert_row(earmattbl, row, row_size)) {
					return -MIDORIDB_INTERNAL;
				}
			}
		}
	}

	return MIDORIDB_OK;
}

static bool cpy_cols(struct table *src, struct row *src_row, struct table *dst, struct row *dst_row)
{
	struct column *column;
	char key[FQFIELD_NAME_LEN];
	size_t offset, pos = 0;

	for (int i = 0; i < src->column_count; i++) {
		column = &src->columns[i];

		memzero(key, sizeof(key));
		snprintf(key, sizeof(key) - 1, "%s.%s", src->name, column->name);

		/* find offset for target table */
		offset = 0;
		for (int j = 0; j < dst->column_count; j++) {
			if (strcmp(dst->columns[j].name, key) == 0)
				break;
			offset += table_calc_column_space(&dst->columns[j]);
		}

		if (table_check_var_column(column)) {
			void *ptr = zalloc(column->precision);

			if (!ptr)
				return false;

			/* Copy data only if column is not NULL */
			if (!bit_test(src_row->null_bitmap, i, sizeof(src_row->null_bitmap))) {
				memcpy(ptr, *((char**)((char*)src_row->data + pos)), column->precision);
			}

			uintptr_t *col_idx_ptr = (uintptr_t*)&dst_row->data[offset];
			*col_idx_ptr = (uintptr_t)ptr;

		} else {
			memcpy(dst_row->data + offset, ((char*)src_row->data) + pos, column->precision);
		}

		pos += table_calc_column_space(column);
	}

	return true;
}

static int merge_rows(struct database *db, struct ast_sel_table_node *left_node, struct ast_sel_table_node *right_node,
		struct row *row_1, struct row *row_2, struct table *mattbl, struct row **out)
{
	struct table *tbl_1, *tbl_2;

	tbl_1 = database_table_get(db, left_node->table_name);
	tbl_2 = database_table_get(db, right_node->table_name);

	*out = zalloc(sizeof(**out));
	if (!(*out))
		goto err;

	(*out)->flags.deleted = false;
	(*out)->flags.empty = false;

	for (int i = 0; i < mattbl->column_count; i++) {
		bit_set((*out)->null_bitmap, i, sizeof((*out)->null_bitmap));
	}

	if (!cpy_cols(tbl_1, row_1, mattbl, *out))
		goto err_ptr;

	if (!cpy_cols(tbl_2, row_2, mattbl, *out))
		goto err_ptr;

	return MIDORIDB_OK;

err_ptr:
	free(*out);
err:
	return -MIDORIDB_INTERNAL;

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

static bool cmp_field_to_field(struct table *table, struct row *row, struct ast_sel_cmp_node *node, struct ast_sel_exprval_node *val_1, struct ast_sel_exprval_node *val_2)
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

static bool cmp_field_to_value(struct table *table, struct row *row, struct ast_sel_cmp_node *node,
		struct ast_sel_exprval_node *field, struct ast_sel_exprval_node *value)
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

static bool cmp_value_to_value(struct ast_sel_cmp_node *node, struct ast_sel_exprval_node *val_1, struct ast_sel_exprval_node *val_2)
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

static bool eval_cmp(struct table *table, struct row *row, struct ast_sel_cmp_node *node)
{
	struct list_head *pos;
	struct ast_node *tmp_node;
	struct ast_sel_exprval_node *val_1 = NULL, *val_2 = NULL;

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

static bool _eval_join_on_clause(struct ast_sel_join_node *join_node, struct ast_node *node,
		struct table *table, struct row *row)
{
	struct list_head *pos;
	struct ast_node *tmp_node;
	struct ast_sel_logop_node *logop_node;
	bool ret = true;

	if (node->node_type == AST_TYPE_SEL_CMP) {
		return eval_cmp(table, row, (struct ast_sel_cmp_node*)node);
	} else if (node->node_type == AST_TYPE_SEL_LOGOP) {
		logop_node = (typeof(logop_node))node;

		bool gate_init = false;

		list_for_each(pos, logop_node->node_children_head)
		{
			tmp_node = list_entry(pos, typeof(*tmp_node), head);

			bool eval = _eval_join_on_clause(join_node, tmp_node, table, row);

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

	} else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_node = list_entry(pos, typeof(*tmp_node), head);

			if (!_eval_join_on_clause(join_node, tmp_node, table, row))
				return false;
		}
	}
	return ret;
}

static int _join_nested_loop(struct database *db, struct ast_sel_join_node *join_node,
		struct ast_sel_onexpr_node *onexpr_node, struct ast_sel_table_node *left_node,
		struct ast_sel_table_node *right_node, struct table *mattbl)
{
	struct table *left, *right;
	struct list_head *left_pos, *right_pos;
	struct datablock *left_blk, *right_blk;
	size_t left_row_size, right_row_size, new_row_size;
	struct row *left_row, *right_row, *new_row;

	left = database_table_get(db, left_node->table_name);
	right = database_table_get(db, right_node->table_name);

	left_row_size = table_calc_row_size(left);
	right_row_size = table_calc_row_size(right);
	new_row_size = table_calc_row_size(mattbl);

	// TODO add nested-loop for other join types.. for now I will focus on the INNER JOIN
	BUG_ON(join_node->join_type != AST_SEL_JOIN_INNER);

	list_for_each(left_pos, left->datablock_head)
	{
		left_blk = list_entry(left_pos, typeof(*left_blk), head);
		for (size_t i = 0; i < (DATABLOCK_PAGE_SIZE / left_row_size); i++) {
			left_row = (struct row*)&left_blk->data[left_row_size * i];

			list_for_each(right_pos, right->datablock_head)
			{
				right_blk = list_entry(right_pos, typeof(*right_blk), head);

				for (size_t j = 0; j < (DATABLOCK_PAGE_SIZE / right_row_size); j++) {
					right_row = (struct row*)&right_blk->data[right_row_size * j];

					/* do we need to compare those rows? */
					if (left_row->flags.deleted || left_row->flags.empty
							|| right_row->flags.deleted
							|| right_row->flags.empty)
						continue;

					//TODO figure out how I will populate aliases into the new row
					// SELECT (f1 + 4) as val FROM A JOIN B ON ....

					/* copy columns of both rows to materialised row */
					if (merge_rows(db, left_node, right_node, left_row, right_row,
							mattbl,
							&new_row))
						goto err;

					if (_eval_join_on_clause(join_node, (struct ast_node*)onexpr_node, mattbl,
									new_row)) {

						if (!table_insert_row(mattbl, new_row, new_row_size))
							goto err;
					}
				}
			}
		}
	}

	return MIDORIDB_OK;

err:
	return -MIDORIDB_INTERNAL;

}

static int proc_from_clause_join(struct database *db, struct ast_sel_join_node *join_node, struct table *earmattbl)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;

	struct ast_sel_onexpr_node *onexpr_node;
	struct ast_node *left_node = NULL, *right_node = NULL;

	list_for_each(pos, join_node->node_children_head)
	{
		tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

		if (tmp_entry->node_type == AST_TYPE_SEL_ONEXPR) {
			onexpr_node = (typeof(onexpr_node))tmp_entry;
		} else if (!left_node) {
			left_node = tmp_entry;
		} else {
			right_node = tmp_entry;
		}
	}

	if (left_node->node_type == AST_TYPE_SEL_TABLE && right_node->node_type == AST_TYPE_SEL_TABLE) {
		return _join_nested_loop(db, join_node, onexpr_node,
						(struct ast_sel_table_node*)left_node,
						(struct ast_sel_table_node*)right_node, earmattbl);
	} else {
		//TODO add other variations (J - J, J - T, T - J)
		BUG_GENERIC();
	}

	return -MIDORIDB_INTERNAL;
}

static int proc_from_clause(struct database *db, struct ast_node *node, struct table *earmattbl)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;

	list_for_each(pos, node->node_children_head)
	{
		tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

		if (tmp_entry->node_type == AST_TYPE_SEL_TABLE) {
			// single table: SELECT * FROM A;
			return proc_from_clause_table(db, (struct ast_sel_table_node*)tmp_entry, earmattbl);
		} else if (tmp_entry->node_type == AST_TYPE_SEL_JOIN) {
			/* at least 1 join is found on the FROM-clause.
			 * additionally, multiple tables are wrapped in synthetic join nodes at the optimisation phase).
			 * this makes the executor's code slightly simpler
			 */
			return proc_from_clause_join(db, (struct ast_sel_join_node*)tmp_entry, earmattbl);
		}
	}

	return -MIDORIDB_INTERNAL;
}

int executor_run_select_stmt(struct database *db, struct ast_sel_select_node *select_node, struct query_output *output)
{
	struct hashtable cols_ht = {0};
	struct table *table = NULL;
	int ret = MIDORIDB_OK;

	if (!hashtable_init(&cols_ht, &hashtable_str_compare, &hashtable_str_hash)) {
		snprintf(output->error.message, sizeof(output->error.message), "execution phase: internal error\n");
		ret = -MIDORIDB_INTERNAL;
		goto err;
	}

	/* build columns hashtable */
	if ((ret = build_cols_hashtable(db, (struct ast_node*)select_node, &cols_ht))) {
		snprintf(output->error.message, sizeof(output->error.message),
				"execution phase: cannot build columns hashtable\n");
		goto err_bld_ht;
	}

	/* build early-materialisation table structure */
	if ((ret = build_table_scafold(&cols_ht, &table))) {
		snprintf(output->error.message, sizeof(output->error.message),
				"execution phase: cannot build early materialisation table\n");
		goto err_bld_ht;
	}

	/* fill out early-mat table with data from the FROM-clause */
	if ((ret = proc_from_clause(db, (struct ast_node*)select_node, table))) {
		snprintf(output->error.message, sizeof(output->error.message),
				"execution phase: error while processing FROM-clause\n");
		goto err_bld_fc;
	}

// temp
	output->results.table = table;

	hashtable_foreach(&cols_ht, &free_hashmap_entries, NULL);
	hashtable_free(&cols_ht);

	return ret;

err_bld_fc:
	table_destroy(&table);
err_bld_ht:
	hashtable_foreach(&cols_ht, &free_hashmap_entries, NULL);
	hashtable_free(&cols_ht);

err:
	return ret;

}
