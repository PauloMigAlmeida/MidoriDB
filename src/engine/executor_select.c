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

static int _build_coltypes_fieldname(struct database *db, struct ast_node *node, struct hashtable *column_types)
{
	struct ast_sel_fieldname_node *field_node;
	struct table *table;
	struct column *column;
	char key[FQFIELD_NAME_LEN];

	field_node = (typeof(field_node))node;
	table = database_table_get(db, field_node->table_name);

	for (int i = 0; i < table->column_count; i++) {
		column = &table->columns[i];

		memzero(key, sizeof(key));
		snprintf(key, sizeof(key) - 1, "%s.%s", field_node->table_name, column->name);

		if (!hashtable_put(column_types, key, strlen(key) + 1, &column->type, sizeof(column->type)))
			return -MIDORIDB_INTERNAL;
	}
	return MIDORIDB_OK;
}

static int _build_coltypes_alias(struct database *db, struct ast_node *node, struct ast_sel_alias_node *alias_node,
		struct hashtable *column_types)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_exprval_node *val_node;
	enum COLUMN_TYPE col_type;
	int ret = MIDORIDB_OK;

	if (node->node_type == AST_TYPE_SEL_EXPRVAL) {
		val_node = (typeof(val_node))node;

		/* sanity check - there shouldn't be any exprvals (name) at this point in here */
		BUG_ON(val_node->value_type.is_name);

		if (val_node->value_type.is_approxnum)
			col_type = CT_DOUBLE;
		else if (val_node->value_type.is_intnum)
			col_type = CT_INTEGER;
		else if (val_node->value_type.is_bool)
			col_type = CT_TINYINT;
		else if (val_node->value_type.is_str)
			col_type = CT_VARCHAR; // PS: raw values are not auto-boxed into CT_DATE/CT_DATETIMEs
		else
			BUG_GENERIC();

		if (!hashtable_put(column_types, alias_node->alias_value, strlen(alias_node->alias_value) + 1,
					&col_type,
					sizeof(col_type)))
			ret = -MIDORIDB_INTERNAL;

	} else if (node->node_type == AST_TYPE_SEL_FIELDNAME) {
		ret = _build_coltypes_fieldname(db, node, column_types);
	} else {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			return _build_coltypes_alias(db, tmp_entry, (struct ast_sel_alias_node*)tmp_entry, column_types);
		}
	}

	return ret;
}

static int build_coltypes_hashtable(struct database *db, struct ast_node *node, struct hashtable *column_types)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	int ret = MIDORIDB_OK;

	list_for_each(pos, node->node_children_head)
	{
		tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

		if (tmp_entry->node_type == AST_TYPE_SEL_FIELDNAME) {
			if ((ret = _build_coltypes_fieldname(db, tmp_entry, column_types)))
				break;
		} else if (tmp_entry->node_type == AST_TYPE_SEL_ALIAS) {
			if ((ret = _build_coltypes_alias(db, tmp_entry, (struct ast_sel_alias_node*)tmp_entry,
								column_types)))
				break;
		}
		/* anything different from that is most likely not columns in the SELECT statement */
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
	column.type = *(enum COLUMN_TYPE*)value;

	BUG_ON(!table_add_column(table, &column));
}

static int build_table_scafold(struct hashtable *column_types, struct table *out_table)
{
	out_table = table_init("early_mat_tbl");

	if (!out_table)
		return -MIDORIDB_INTERNAL;

	hashtable_foreach(column_types, &do_build_table_scafold, out_table);

	return MIDORIDB_OK;
}

static int proc_from_clause(struct database *db, struct ast_node *node, struct query_output *output, struct table *earmattbl)
{
	return MIDORIDB_OK;
}

int executor_run_select_stmt(struct database *db, struct ast_sel_select_node *select_node, struct query_output *output)
{
	struct hashtable column_types = {0};
	struct table *table = NULL;
	int ret = MIDORIDB_OK;

	if (!hashtable_init(&column_types, &hashtable_str_compare, &hashtable_str_hash)) {
		snprintf(output->error.message, sizeof(output->error.message), "execution phase: internal error\n");
		ret = -MIDORIDB_INTERNAL;
		goto err;
	}

	/* build column-types hashtable */
	if ((ret = build_coltypes_hashtable(db, (struct ast_node*)select_node, &column_types))) {
		snprintf(output->error.message, sizeof(output->error.message),
				"execution phase: cannot build column tables hashtable\n");
		goto err_bld_ht;
	}

	/* build early-materialisation table structure */
	if ((ret = build_table_scafold(&column_types, table))) {
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

	hashtable_foreach(&column_types, &free_hashmap_entries, NULL);
	hashtable_free(&column_types);

	return ret;

err_bld_fc:
	table_destroy(&table);
err_bld_ht:
	hashtable_foreach(&column_types, &free_hashmap_entries, NULL);
	hashtable_free(&column_types);

err:
	return ret;

}
