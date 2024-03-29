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

//static void print_row(struct table *table, struct row *row)
//{
//	struct column *column;
//	size_t offset = 0;
//
//	printf("Row: { ");
//
//	for (int i = 0; i < table->column_count; i++) {
//		column = &table->columns[i];
//		printf(".%s = ", column->name);
//		printf("%ld , ", *(int64_t*)&row->data[offset]);
//		offset += table_calc_column_space(column);
//	}
//
//	printf("};\n");
//}

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

static int int_compare(const void *a, const void *b)
{
	return *((int*)a) - *((int*)b);
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

struct column_specs {
	enum COLUMN_TYPE type;
	size_t offset;
	int col_idx;
};

static bool find_column_mattbl(struct table *table, struct ast_node *node, struct column_specs *out)
{
	char key[FQFIELD_NAME_LEN] = {0};
	bool found = false;

	struct ast_sel_fieldname_node *fld_node;
	struct ast_sel_exprval_node *val_node;

	for (int i = 0; i < table->column_count; i++) {
		struct column *col = &table->columns[i];

		if (node->node_type == AST_TYPE_SEL_EXPRVAL) {
			val_node = (typeof(val_node))node;

			BUG_ON(!val_node->value_type.is_name);

			memzero(key, sizeof(key));
			strncpy(key, val_node->name_val, sizeof(key) - 1);
		} else if (node->node_type == AST_TYPE_SEL_FIELDNAME) {
			fld_node = (typeof(fld_node))node;

			memzero(key, sizeof(key));
			snprintf(key, sizeof(key) - 1, "%s.%s", fld_node->table_name, fld_node->col_name);
		} else if (node->node_type == AST_TYPE_SEL_COUNT) {
			memzero(key, sizeof(key));
			strcpy(key, "COUNT(*)");
		} else {
			BUG_GENERIC();
		}

		if (strcmp(col->name, key) == 0) {
			found = true;
			out->type = col->type;
			out->col_idx = i;
			break;
		}

		out->offset += table_calc_column_space(col);
	}

	return found;
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

static int _build_cols_hashtable_count(struct ast_node *node, struct hashtable *cols_ht)
{
	UNUSED(node);
	char key[] = "COUNT(*)";
	struct column column = {0};

	column.type = CT_INTEGER;
	column.precision = table_calc_column_precision(column.type);
	column.is_count = true;

	if (!hashtable_put(cols_ht, key, strlen(key) + 1, &column, sizeof(column)))
		return -MIDORIDB_INTERNAL;

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
	} else if (node->node_type == AST_TYPE_SEL_COUNT) {
		return _build_cols_hashtable_count(node, cols_ht);
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
	column.is_count = ((struct column*)value)->is_count;

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

static void init_count_cols(struct table *table, struct row *row)
{
	struct column *column;
	size_t offset = 0;

	for (int i = 0; i < table->column_count; i++) {
		column = &table->columns[i];

		if (column->is_count) {
			(*(int64_t*)&row->data[offset]) = 1;
		}

		offset += table_calc_column_space(column);
	}
}

static bool cpy_cols(struct table *src, struct row *src_row, struct table *dst, struct row *dst_row, bool is_src_earlymat)
{
	struct column *column;
	char key[FQFIELD_NAME_LEN];
	size_t dst_offset, src_offset = 0;
	int dst_col_idx = -1;

	for (int i = 0; i < src->column_count; i++) {
		column = &src->columns[i];

		memzero(key, sizeof(key));

		if (!is_src_earlymat) {
			snprintf(key, sizeof(key) - 1, "%s.%s", src->name, column->name);
		} else {
			strncpy(key, column->name, sizeof(key) - 1);
		}

		/* find offset for target table */
		dst_offset = 0;
		for (int j = 0; j < dst->column_count; j++) {
			if (strcmp(dst->columns[j].name, key) == 0) {
				dst_col_idx = j;
				break;
			}
			dst_offset += table_calc_column_space(&dst->columns[j]);
		}

		if (table_check_var_column(column)) {
			void *ptr = zalloc(column->precision);

			if (!ptr)
				return false;

			/* Copy data only if column is not NULL */
			if (!bit_test(src_row->null_bitmap, i, sizeof(src_row->null_bitmap))) {
				memcpy(ptr, *((char**)((char*)src_row->data + src_offset)), column->precision);
			}

			uintptr_t *col_idx_ptr = (uintptr_t*)&dst_row->data[dst_offset];
			*col_idx_ptr = (uintptr_t)ptr;

		} else {

			if (!bit_test(src_row->null_bitmap, i, sizeof(src_row->null_bitmap))) {
				memcpy(dst_row->data + dst_offset, ((char*)src_row->data) + src_offset,
					column->precision);
			}

		}

		/* adjust null_bitmap */
		if (!bit_test(src_row->null_bitmap, i, sizeof(src_row->null_bitmap))) {
			bit_clear(dst_row->null_bitmap, dst_col_idx, sizeof(dst_row->null_bitmap));
		}

		src_offset += table_calc_column_space(column);
	}

	return true;
}

static int _merge_rows(struct table *tbl_1, struct table *tbl_2, struct row *row_1, struct row *row_2,
		struct table *mattbl, struct row **out, bool is_tbl2_earlymat)
{
	*out = zalloc(table_calc_row_size(mattbl));
	if (!(*out))
		goto err;

	(*out)->flags.deleted = false;
	(*out)->flags.empty = false;

	for (int i = 0; i < mattbl->column_count; i++) {
		bit_set((*out)->null_bitmap, i, sizeof((*out)->null_bitmap));
	}

	if (!cpy_cols(tbl_1, row_1, mattbl, *out, false))
		goto err_ptr;

	if (!cpy_cols(tbl_2, row_2, mattbl, *out, is_tbl2_earlymat))
		goto err_ptr;

	init_count_cols(mattbl, *out);

	return MIDORIDB_OK;

err_ptr:
	free(*out);
err:
	return -MIDORIDB_INTERNAL;

}

static int merge_rows(struct table *tbl_1, struct table *tbl_2, struct row *row_1, struct row *row_2,
		struct table *mattbl, struct row **out)
{
	bool is_tbl2_earlymat = tbl_2 == mattbl;
	return _merge_rows(tbl_1, tbl_2, row_1, row_2, mattbl, out, is_tbl2_earlymat);
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

static bool cmp_field_to_field(struct table *table, struct row *row, struct ast_sel_cmp_node *node,
		struct ast_sel_exprval_node *val_1, struct ast_sel_exprval_node *val_2)
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

static bool cmp_fieldname_to_fieldname(struct table *table, struct row *row, struct ast_sel_cmp_node *node,
		struct ast_sel_fieldname_node *val_1, struct ast_sel_fieldname_node *val_2)
{
	enum COLUMN_TYPE type = 0;
	size_t offset_1 = 0, offset_2 = 0;
	int col_idx_1 = -1, col_idx_2 = -1;
	bool is_null_1, is_null_2;
	char key[FQFIELD_NAME_LEN];

	for (int i = 0, tmp_offset = 0; i < table->column_count; i++) {
		struct column *col = &table->columns[i];

		memzero(key, sizeof(key));
		snprintf(key, sizeof(key) - 1, "%s.%s", val_1->table_name, val_1->col_name);

		if (strcmp(col->name, key) == 0) {
			/* semantic phase guarantees that columns will have the same type in CMP nodes */
			type = col->type;
			offset_1 = tmp_offset;
			col_idx_1 = i;
			break;
		}
		tmp_offset += table_calc_column_space(col);
	}

	for (int i = 0, tmp_offset = 0; i < table->column_count; i++) {
		struct column *col = &table->columns[i];

		memzero(key, sizeof(key));
		snprintf(key, sizeof(key) - 1, "%s.%s", val_2->table_name, val_2->col_name);

		if (strcmp(col->name, key) == 0) {
			offset_2 = tmp_offset;
			col_idx_2 = i;
			break;
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

static bool cmp_fieldname_to_value(struct table *table, struct row *row, struct ast_sel_cmp_node *node,
		struct ast_sel_fieldname_node *field, struct ast_sel_exprval_node *value)
{
	enum COLUMN_TYPE type = 0;
	size_t offset = 0;
	int col_idx = -1;
	bool is_null;
	char key[FQFIELD_NAME_LEN];

	for (int i = 0; i < table->column_count; i++) {
		struct column *col = &table->columns[i];

		memzero(key, sizeof(key));
		snprintf(key, sizeof(key) - 1, "%s.%s", field->table_name, field->col_name);

		if (strcmp(col->name, key) == 0) {
			/* semantic phase guarantees that columns will have the same type in CMP nodes */
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

static bool cmp_value_to_fieldname(struct table *table, struct row *row, struct ast_sel_cmp_node *node,
		struct ast_sel_exprval_node *value, struct ast_sel_fieldname_node *field)
{
	enum COLUMN_TYPE type = 0;
	size_t offset = 0;
	int col_idx = -1;
	bool is_null;
	char key[FQFIELD_NAME_LEN];

	for (int i = 0; i < table->column_count; i++) {
		struct column *col = &table->columns[i];

		memzero(key, sizeof(key));
		snprintf(key, sizeof(key) - 1, "%s.%s", field->table_name, field->col_name);

		if (strcmp(col->name, key) == 0) {
			/* semantic phase guarantees that columns will have the same type in CMP nodes */
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
		return cmp_double_value_to_value(node->cmp_type, value->double_val, *(double*)&row->data[offset]);
	} else if (type == CT_TINYINT) {
		return cmp_bool_value_to_value(node->cmp_type, value->bool_val, *(bool*)&row->data[offset]);
	} else if (type == CT_INTEGER) {
		return cmp_int_value_to_value(node->cmp_type, value->int_val, *(int64_t*)&row->data[offset]);
	} else if (type == CT_DATE || type == CT_DATETIME) {
		return cmp_time_value_to_value(node->cmp_type,
						parse_date_type(value->str_val, type),
						*(time_t*)&row->data[offset]);
	} else if (type == CT_VARCHAR) {
		return cmp_str_value_to_value(node->cmp_type, value->str_val, *(char**)&row->data[offset]);
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
	struct ast_node *node_1 = NULL, *node_2 = NULL;
	struct ast_sel_exprval_node *val_1, *val_2;
	struct ast_sel_fieldname_node *fld_1, *fld_2;

	list_for_each(pos, node->node_children_head)
	{
		tmp_node = list_entry(pos, typeof(*tmp_node), head);

		if (!node_1)
			node_1 = tmp_node;
		else
			node_2 = tmp_node;
	}

	if (node_1->node_type == node_2->node_type && node_1->node_type == AST_TYPE_SEL_EXPRVAL) {
		val_1 = (typeof(val_1))node_1;
		val_2 = (typeof(val_2))node_2;

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
	} else if (node_1->node_type == node_2->node_type && node_1->node_type == AST_TYPE_SEL_FIELDNAME) {
		fld_1 = (typeof(fld_1))node_1;
		fld_2 = (typeof(fld_2))node_2;

		return cmp_fieldname_to_fieldname(table, row, node, fld_1, fld_2);
	} else if (node_1->node_type == AST_TYPE_SEL_FIELDNAME && node_2->node_type == AST_TYPE_SEL_EXPRVAL) {
		fld_1 = (typeof(fld_1))node_1;
		val_2 = (typeof(val_2))node_2;

		return cmp_fieldname_to_value(table, row, node, fld_1, val_2);
	} else if (node_1->node_type == AST_TYPE_SEL_EXPRVAL && node_2->node_type == AST_TYPE_SEL_FIELDNAME) {
		val_1 = (typeof(val_1))node_1;
		fld_2 = (typeof(fld_2))node_2;

		return cmp_value_to_fieldname(table, row, node, val_1, fld_2);
	} else {
		/* to be implemented */
		BUG_GENERIC();
		return false;
	}

}

static bool eval_isxnull(struct table *table, struct row *row, struct ast_sel_isxnull_node *node)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_fieldname_node *field = NULL;
	struct ast_sel_exprval_node *val = NULL;
	int col_idx = -1;
	bool is_field = false;
	char key[FQFIELD_NAME_LEN] = {0};

	list_for_each(pos, node->node_children_head)
	{

		tmp_entry = list_entry(pos, typeof(*tmp_entry), head);
		if (tmp_entry->node_type == AST_TYPE_SEL_FIELDNAME) {
			field = (typeof(field))tmp_entry;
			is_field = true;
			break;
		} else if (tmp_entry->node_type == AST_TYPE_SEL_EXPRVAL) {
			val = (typeof(val))tmp_entry;
			break;
		}
	}

	/* sanity check */
	BUG_ON(!field && !val);

	if (is_field) {
		snprintf(key, sizeof(key) - 1, "%s.%s", field->table_name, field->col_name);
	} else {
		strncpy(key, val->name_val, sizeof(key) - 1);
	}

	for (int i = 0; i < table->column_count; i++) {
		struct column *col = &table->columns[i];

		if (strcmp(col->name, key) == 0) {
			col_idx = i;
			break;
		}
	}

	return bit_test(row->null_bitmap, col_idx, sizeof(row->null_bitmap)) ^ node->is_negation;
}

static bool eval_isxin(struct table *table, struct row *row, struct ast_sel_isxin_node *node)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_fieldname_node *field = NULL;
	struct ast_sel_exprval_node *value = NULL;
	struct ast_sel_cmp_node cmp_node = {0};
	bool is_field;

	if (node->is_negation) {
		cmp_node.cmp_type = AST_CMP_DIFF_OP;
	} else {
		cmp_node.cmp_type = AST_CMP_EQUALS_OP;
	}

	list_for_each(pos, node->node_children_head)
	{
		tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

		if (tmp_entry->node_type == AST_TYPE_SEL_EXPRVAL) {
			if (((struct ast_sel_exprval_node*)tmp_entry)->value_type.is_name) {
				value = (typeof(value))tmp_entry;
				break;
			}
		} else if (tmp_entry->node_type == AST_TYPE_SEL_FIELDNAME) {
			field = (typeof(field))tmp_entry;
			is_field = true;
			break;
		}
	}

	/* sanity check */
	BUG_ON(!field && !value);

	list_for_each(pos, node->node_children_head)
	{
		tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

		if (tmp_entry->node_type == AST_TYPE_SEL_FIELDNAME)
			continue; /*same as field we jus fetched.. just move on */

		BUG_ON(tmp_entry->node_type != AST_TYPE_SEL_EXPRVAL);

		if (!((struct ast_sel_exprval_node*)tmp_entry)->value_type.is_name) {

			if (is_field) {
				if (!cmp_fieldname_to_value(table, row, &cmp_node, field,
								(struct ast_sel_exprval_node*)tmp_entry))
					return false; // fail-fast
			} else {
				if (!cmp_field_to_value(table, row, &cmp_node, value,
							(struct ast_sel_exprval_node*)tmp_entry))
					return false; // fail-fast
			}
		}
	}
	return true;
}

static bool eval_row_cond(struct ast_node *node, struct table *table, struct row *row)
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

			bool eval = eval_row_cond(tmp_node, table, row);

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

	} else if (node->node_type == AST_TYPE_SEL_EXPRISXIN) {
		return eval_isxin(table, row, (struct ast_sel_isxin_node*)node);
	} else if (node->node_type == AST_TYPE_SEL_EXPRISXNULL) {
		return eval_isxnull(table, row, (struct ast_sel_isxnull_node*)node);
	} else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_node = list_entry(pos, typeof(*tmp_node), head);

			if (!eval_row_cond(tmp_node, table, row))
				return false;
		}
	}
	return ret;
}

static int _join_nested_loop_tbl2tbl(struct database *db, struct ast_sel_join_node *join_node,
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

			if (left_row->flags.empty)
				break; /* end of the line */

			if (left_row->flags.deleted)
				continue; /* nothing to do here */

			list_for_each(right_pos, right->datablock_head)
			{
				right_blk = list_entry(right_pos, typeof(*right_blk), head);

				for (size_t j = 0; j < (DATABLOCK_PAGE_SIZE / right_row_size); j++) {
					right_row = (struct row*)&right_blk->data[right_row_size * j];

					if (right_row->flags.empty)
						break; /* end of the line */

					if (right_row->flags.deleted)
						continue; /* nothing to do here */

					//TODO figure out how I will populate aliases into the new row
					// SELECT (f1 + 4) as val FROM A JOIN B ON ....

					/* copy columns of both rows to materialised row */
					if (merge_rows(left, right, left_row, right_row, mattbl, &new_row))
						goto err;

					if (eval_row_cond((struct ast_node*)onexpr_node, mattbl, new_row)) {

						if (!table_insert_row(mattbl, new_row, new_row_size))
							goto err;

//						print_row(mattbl, new_row);

					}
					/* free up used resources */
					table_free_row_content(mattbl, new_row);
					free(new_row);
				}
			}
		}
	}

	return MIDORIDB_OK;

err:
	return -MIDORIDB_INTERNAL;

}

static int _join_nested_loop_tbl2mat(struct database *db, struct ast_sel_join_node *join_node,
		struct ast_sel_onexpr_node *onexpr_node, struct ast_sel_table_node *table_node_1,
		struct table *mattbl)
{
	struct table *table_1, *table_2;
	struct list_head *tbl1_pos, *tbl2_pos;
	struct datablock *tbl1_blk, *tbl2_blk;
	size_t tbl1_row_size, tbl2_row_size, new_row_size;
	struct row *tbl1_row, *tbl2_row, *new_row;

	table_1 = database_table_get(db, table_node_1->table_name);
	table_2 = mattbl;

	tbl1_row_size = table_calc_row_size(table_1);
	tbl2_row_size = table_calc_row_size(table_2);
	new_row_size = table_calc_row_size(mattbl);

	// TODO add nested-loop for other join types.. for now I will focus on the INNER JOIN
	BUG_ON(join_node->join_type != AST_SEL_JOIN_INNER);

	list_for_each(tbl1_pos, table_1->datablock_head)
	{
		tbl1_blk = list_entry(tbl1_pos, typeof(*tbl1_blk), head);
		for (size_t i = 0; i < (DATABLOCK_PAGE_SIZE / tbl1_row_size); i++) {
			tbl1_row = (struct row*)&tbl1_blk->data[tbl1_row_size * i];

			if (tbl1_row->flags.empty)
				break; /* end of the line */

			if (tbl1_row->flags.deleted)
				continue; /* nothing to do here */

			list_for_each(tbl2_pos, table_2->datablock_head)
			{
				tbl2_blk = list_entry(tbl2_pos, typeof(*tbl2_blk), head);

				for (size_t j = 0; j < (DATABLOCK_PAGE_SIZE / tbl2_row_size); j++) {
					tbl2_row = (struct row*)&tbl2_blk->data[tbl2_row_size * j];

					if (tbl2_row->flags.empty)
						break; /* end of the line */

					if (tbl2_row->flags.deleted)
						continue; /* nothing to do here */

					//TODO figure out how I will populate aliases into the new row
					// SELECT (f1 + 4) as val FROM A JOIN B ON ....

					/* copy columns of both rows to materialised row */
					if (merge_rows(table_1, mattbl, tbl1_row, tbl2_row, mattbl, &new_row))
						goto err;

					if (eval_row_cond((struct ast_node*)onexpr_node, mattbl, new_row)) {

						if (!table_update_row(mattbl, tbl2_blk, tbl2_row_size * j, new_row,
									new_row_size))
							goto err;

//						print_row(mattbl, new_row);

					} else {
						/* when using materialisation tables,
						 * if it doesn't match ON-clause then it needs to be discarded
						 * TODO: this should change once we implement other JOIN types
						 */
						table_delete_row(mattbl, tbl2_blk, tbl2_row_size * j);
					}

					/* free up used resources */
					table_free_row_content(mattbl, new_row);
					free(new_row);
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

	struct ast_sel_onexpr_node *onexpr_node = NULL;
	struct ast_node *left_node = NULL, *right_node = NULL;

	int early_ret;

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
		return _join_nested_loop_tbl2tbl(db, join_node, onexpr_node,
							(struct ast_sel_table_node*)left_node,
							(struct ast_sel_table_node*)right_node, earmattbl);
	} else if (left_node->node_type == AST_TYPE_SEL_JOIN && right_node->node_type == AST_TYPE_SEL_TABLE) {
		if ((early_ret = proc_from_clause_join(db, (struct ast_sel_join_node*)left_node, earmattbl)))
			return early_ret;

		return _join_nested_loop_tbl2mat(db, join_node, onexpr_node,
							(struct ast_sel_table_node*)right_node,
							earmattbl);
	} else if (left_node->node_type == AST_TYPE_SEL_TABLE && right_node->node_type == AST_TYPE_SEL_JOIN) {
		if ((early_ret = proc_from_clause_join(db, (struct ast_sel_join_node*)right_node, earmattbl)))
			return early_ret;

		return _join_nested_loop_tbl2mat(db, join_node, onexpr_node,
							(struct ast_sel_table_node*)left_node,
							earmattbl);
	} else {
		BUG_GENERIC();
	}

	return MIDORIDB_OK;
}

static int proc_from_clause_table(struct database *db, struct ast_sel_table_node *table_node, struct table *mattbl)
{
	struct list_head *pos;
	struct table *exs_table;
	struct datablock *exs_block;
	struct row *exs_row, *new_row;
	size_t exs_row_size, new_row_size;

	exs_table = database_table_get(db, table_node->table_name);

	exs_row_size = table_calc_row_size(exs_table);
	new_row_size = table_calc_row_size(mattbl);

	list_for_each(pos, exs_table->datablock_head)
	{
		exs_block = list_entry(pos, typeof(*exs_block), head);

		for (size_t i = 0; i < (DATABLOCK_PAGE_SIZE / exs_row_size); i++) {
			exs_row = (typeof(exs_row))&exs_block->data[exs_row_size * i];

			if (exs_row->flags.empty)
				break; /* end of line */

			if (exs_row->flags.deleted)
				continue; /* nothing to do here */

			/* rebuild row - needed queries that use COUNT(*) */
			new_row = zalloc(new_row_size);
			if (!new_row)
				goto err;

			new_row->flags.deleted = false;
			new_row->flags.empty = false;

			for (int i = 0; i < mattbl->column_count; i++) {
				bit_set(new_row->null_bitmap, i, sizeof(new_row->null_bitmap));
			}

			if (!cpy_cols(exs_table, exs_row, mattbl, new_row, false))
				goto err_ptr;

			init_count_cols(mattbl, new_row);

			if (!table_insert_row(mattbl, new_row, new_row_size)) {
				goto err_ins;
			}

//			print_row(mattbl, new_row);
			table_free_row_content(mattbl, new_row);
			free(new_row);
		}
	}

	return MIDORIDB_OK;

err_ins:
	table_free_row_content(mattbl, new_row);
err_ptr:
	free(new_row);
err:
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

static int proc_select_clause(struct ast_sel_select_node *node, struct table *outtbl)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct column *column;
	struct ast_sel_exprval_node *val_node;
	struct ast_sel_fieldname_node *fld_node;
	int rem_col[TABLE_MAX_COLUMNS] = {0};
	int rem_col_idx = -1;
	char key[FQFIELD_NAME_LEN];
	bool found;

	for (int i = 0; i < outtbl->column_count; i++) {
		column = &outtbl->columns[i];
		found = false;

		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (tmp_entry->node_type == AST_TYPE_SEL_EXPRVAL) {
				val_node = (typeof(val_node))tmp_entry;

				//TODO what about raw values ?
				BUG_ON(!val_node->value_type.is_name);

				memzero(key, sizeof(key));
				strncpy(key, val_node->name_val, sizeof(key) - 1);
			} else if (tmp_entry->node_type == AST_TYPE_SEL_FIELDNAME) {
				fld_node = (typeof(fld_node))tmp_entry;

				memzero(key, sizeof(key));
				snprintf(key, sizeof(key) - 1, "%s.%s", fld_node->table_name, fld_node->col_name);
			} else if (tmp_entry->node_type == AST_TYPE_SEL_COUNT) {
				memzero(key, sizeof(key));
				strcpy(key, "COUNT(*)");
			} else {
				/* nothing to just, just move on */
				continue;
			}

			if (strcmp(column->name, key) == 0) {
				found = true;
				break;
			}

		}

		if (!found) {
			rem_col_idx++;
			rem_col[rem_col_idx] = i;
		}
	}

	if (rem_col_idx > -1) {
		qsort(rem_col, rem_col_idx + 1, sizeof(int), int_compare);

		for (int i = rem_col_idx; i >= 0; i--) {
			if (!table_rem_column(outtbl, &outtbl->columns[rem_col[i]]))
				return -MIDORIDB_INTERNAL;
		}
	}

	return MIDORIDB_OK;
}

static int proc_where_clause(struct ast_node *node, struct table *table)
{
	struct list_head *pos;
	struct datablock *blk;
	struct row *row;
	size_t row_size;

	row_size = table_calc_row_size(table);

	list_for_each(pos, table->datablock_head)
	{
		blk = list_entry(pos, typeof(*blk), head);
		for (size_t i = 0; i < (DATABLOCK_PAGE_SIZE / row_size); i++) {
			row = (struct row*)&blk->data[row_size * i];

			if (row->flags.empty)
				break;

			if (row->flags.deleted)
				continue;

			if (!eval_row_cond(node, table, row)) {
				table_delete_row(table, blk, row_size * i);
			}
		}
	}

	return MIDORIDB_OK;
}

static int cmp_rows_col_mattbl(struct table *table, struct row *row_1, struct row *row_2, struct ast_node *fld_val)
{
	struct column_specs col_spcs = {0};
	bool is_null_1, is_null_2;
	int ret = 0;

	BUG_ON(!find_column_mattbl(table, fld_val, &col_spcs));

	is_null_1 = bit_test(row_1->null_bitmap, col_spcs.col_idx, sizeof(row_1->null_bitmap));
	is_null_2 = bit_test(row_2->null_bitmap, col_spcs.col_idx, sizeof(row_2->null_bitmap));

	/* MySQL and SQLITE seems to evaluate values that are NULL to be smaller than everything else... */
	if (is_null_1 && is_null_2)
		ret = 0;
	else if (is_null_1 && !is_null_2)
		ret = -1;
	else if (!is_null_1 && is_null_2)
		ret = 1;
	else if (col_spcs.type == CT_DOUBLE) {
		ret = (*(double*)&row_1->data[col_spcs.offset]) - (*(double*)&row_2->data[col_spcs.offset]);
	} else if (col_spcs.type == CT_TINYINT) {
		ret = (*(bool*)&row_1->data[col_spcs.offset]) - (*(bool*)&row_2->data[col_spcs.offset]);
	} else if (col_spcs.type == CT_INTEGER) {
		ret = (*(int64_t*)&row_1->data[col_spcs.offset]) - (*(int64_t*)&row_2->data[col_spcs.offset]);
	} else if (col_spcs.type == CT_DATE || col_spcs.type == CT_DATETIME) {
		ret = (*(time_t*)&row_1->data[col_spcs.offset]) - (*(time_t*)&row_2->data[col_spcs.offset]);
	} else if (col_spcs.type == CT_VARCHAR) {
		ret = strcmp(*(char**)&row_1->data[col_spcs.offset], *(char**)&row_2->data[col_spcs.offset]);
	} else {
		/* some new column type that I forgot to add in here */
		BUG_GENERIC();
	}

	return ret;
}

static int inc_count_cols(struct table *table, struct datablock *blk, size_t blk_offset, struct row *row, size_t row_size)
{
	struct column *column;
	size_t offset = 0;
	bool found = false;

	for (int i = 0; i < table->column_count; i++) {
		column = &table->columns[i];

		if (column->is_count) {
			found = true;
			(*(int64_t*)&row->data[offset])++;
		}

		offset += table_calc_column_space(column);
	}

	if (found) {
		if (!table_update_row(table, blk, blk_offset, row, row_size))
			return -MIDORIDB_INTERNAL;
	}

	return MIDORIDB_OK;
}

static int proc_groupby_clause(struct ast_node *node, struct table *table)
{
	struct list_head *grppos, *pos1, *pos2;
	struct ast_node *tmp_entry;
	struct datablock *blk_1, *blk_2;
	struct row *row_1, *row_2;
	size_t row_size;
	int ret = MIDORIDB_OK;

	row_size = table_calc_row_size(table);

	list_for_each(grppos, node->node_children_head)
	{
		/* process each field on the group-by clause list one at a time */
		tmp_entry = list_entry(grppos, typeof(*tmp_entry), head);

		list_for_each(pos1, table->datablock_head)
		{
			blk_1 = list_entry(pos1, typeof(*blk_1), head);
			for (size_t i = 0; i < (DATABLOCK_PAGE_SIZE / row_size); i++) {
				row_1 = (struct row*)&blk_1->data[row_size * i];

				if (row_1->flags.empty)
					break; /* end of the line */

				if (row_1->flags.deleted)
					continue; /* nothing to do here */

				list_for_each(pos2, table->datablock_head)
				{
					blk_2 = list_entry(pos2, typeof(*blk_2), head);
					for (size_t j = i + 1; j < (DATABLOCK_PAGE_SIZE / row_size); j++) {
						row_2 = (struct row*)&blk_2->data[row_size * j];

						if (row_2->flags.empty)
							break; /* end of the line */

						if (row_2->flags.deleted)
							continue; /* nothing to do here */

						if (cmp_rows_col_mattbl(table, row_1, row_2, tmp_entry) == 0) {

							if (!table_delete_row(table, blk_2, row_size * j)) {
								ret = -MIDORIDB_INTERNAL;
								goto out;
							}

							/* increment counts if any */
							if ((ret = inc_count_cols(table, blk_1, row_size * i,
											row_1,
											row_size)))
								goto out;
						}
					}
				}
			}
		}
	}

out:
	return ret;

}

static int handle_countonly_case(struct table *table)
{
	struct list_head *pos1, *pos2;
	struct datablock *blk_1, *blk_2;
	struct row *row_1, *row_2;
	size_t row_size;
	int ret = MIDORIDB_OK;

	/* check if we need to do that in the first plae */
	bool non_count_field = false;
	for (int i = 0; i < table->column_count; i++) {
		if (!table->columns[i].is_count) {
			non_count_field = true;
			break;
		}
	}

	if (!non_count_field) {
		row_size = table_calc_row_size(table);

		list_for_each(pos1, table->datablock_head)
		{
			blk_1 = list_entry(pos1, typeof(*blk_1), head);
			for (size_t i = 0; i < (DATABLOCK_PAGE_SIZE / row_size); i++) {
				row_1 = (struct row*)&blk_1->data[row_size * i];

				if (row_1->flags.empty)
					break; /* end of the line */

				if (row_1->flags.deleted)
					continue; /* nothing to do here */

				list_for_each(pos2, table->datablock_head)
				{
					blk_2 = list_entry(pos2, typeof(*blk_2), head);
					for (size_t j = i + 1; j < (DATABLOCK_PAGE_SIZE / row_size); j++) {
						row_2 = (struct row*)&blk_2->data[row_size * j];

						if (row_2->flags.empty)
							break; /* end of the line */

						if (row_2->flags.deleted)
							continue; /* nothing to do here */

						if (!table_delete_row(table, blk_2, row_size * j)) {
							ret = -MIDORIDB_INTERNAL;
							goto out;
						}

						/* increment counts if any */
						if ((ret = inc_count_cols(table, blk_1, row_size * i,
										row_1,
										row_size)))
							goto out;
					}
				}
			}
		}
	}

out:
	return ret;

}

int executor_run_select_stmt(struct database *db, struct ast_sel_select_node *select_node, struct query_output *output)
{
	struct hashtable cols_ht = {0};
	struct table *table = NULL;
	struct ast_node *where_node, *groupby_node;
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

	/* filter out rows that don't match WHERE-clause */
	where_node = find_node((struct ast_node*)select_node, AST_TYPE_SEL_WHERE);
	if (where_node) {
		if ((ret = proc_where_clause(where_node, table))) {
			snprintf(output->error.message, sizeof(output->error.message),
					"execution phase: error while processing WHERE-clause\n");
			goto err_bld_fc;
		}
	}

	/* collate rows if group-by clause is specified */
	groupby_node = find_node((struct ast_node*)select_node, AST_TYPE_SEL_GROUPBY);
	if (groupby_node) {
		if ((ret = proc_groupby_clause(groupby_node, table))) {
			snprintf(output->error.message, sizeof(output->error.message),
					"execution phase: error while processing GROUPBY-clause\n");
			goto err_bld_fc;
		}
	}

	/* leave only columns that were specified in the statements */
	if ((ret = proc_select_clause(select_node, table))) {
		snprintf(output->error.message, sizeof(output->error.message),
				"execution phase: error while processing SELECT-clause\n");
		goto err_bld_fc;
	}

	/* handle COUNT(*)-only field edge-case */
	if ((ret = handle_countonly_case(table))) {
		snprintf(output->error.message, sizeof(output->error.message),
				"execution phase: error while processing COUNT-only-case\n");
		goto err_bld_fc;
	}

	/* TODO process distinct */

	/* remove excluded rows from ON-clauses, WHERE-clauses and so on */
	table_vacuum(table);

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
