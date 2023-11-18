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

#include <engine/executor.h>
#include <primitive/row.h>

#define FQFIELD_NAME_LEN 	MEMBER_SIZE(struct ast_sel_fieldname_node, table_name)			\
					+ 1 /* dot */ 							\
					+ MEMBER_SIZE(struct ast_sel_fieldname_node, col_name)		\
					+ 1 /* NUL */

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

static int proc_from_clause_single_table(struct database *db, struct ast_sel_table_node *table_node, struct table *earmattbl)
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

			//TODO process WHERE-Clause here?? This can substantially reduce the amount of memory required
			if (!row->flags.deleted && !row->flags.empty /* && should_add_row(table, row, node) */) {
				if (!table_insert_row(earmattbl, row, row_size)) {
					return -MIDORIDB_INTERNAL;
				}
			}
		}
	}

	return MIDORIDB_OK;
}

static int proc_from_clause(struct database *db, struct ast_node *node, struct query_output *output, struct table *earmattbl)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;

	UNUSED(output);

	/*
	 * TODO this logic will have to change a lot to cover all possible scenarios. I'm writing a happy-path one
	 * so I can get the tests/executor_select.c code organised
	 */
	list_for_each(pos, node->node_children_head)
	{
		tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

		if (tmp_entry->node_type == AST_TYPE_SEL_TABLE) {
			return proc_from_clause_single_table(db, (struct ast_sel_table_node*)tmp_entry, earmattbl);
		}
	}

	return MIDORIDB_OK;
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
	if ((ret = proc_from_clause(db, (struct ast_node*)select_node, output, table))) {
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
