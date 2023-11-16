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

static int build_coltypes_hashtable(struct database *db, struct ast_node *node, struct query_output *output,
		struct hashtable *column_types)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	int ret;

	list_for_each(pos, node->node_children_head)
	{
		tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

		if (tmp_entry->node_type == AST_TYPE_SEL_FIELDNAME) {
			if ((ret = _build_coltypes_fieldname(db, tmp_entry, column_types)))
				goto err;
		} else if (tmp_entry->node_type == AST_TYPE_SEL_ALIAS) {
			// TODO add ALIAS handler (might need multiple recursion to get it right)
		}
		/* anything different from that is most likely not columns in the SELECT statement */
	}
	return MIDORIDB_OK;
err:
	snprintf(output->error.message, sizeof(output->error.message),
			"execution phase: cannot build column tables hashtable\n");
	return ret;
}

int executor_run_select_stmt(struct database *db, struct ast_sel_select_node *select_node, struct query_output *output)
{
	struct hashtable column_types = {0};
	int ret = MIDORIDB_OK;

	if (!hashtable_init(&column_types, &hashtable_str_compare, &hashtable_str_hash)) {
		snprintf(output->error.message, sizeof(output->error.message), "execution phase: internal error\n");
		ret = -MIDORIDB_INTERNAL;
		goto out;
	}

	/* build column-types hashtable */
	if ((ret = build_coltypes_hashtable(db, (struct ast_node*)select_node, output, &column_types)))
		goto cleanup;

cleanup:
	hashtable_foreach(&column_types, &free_hashmap_entries, NULL);
	hashtable_free(&column_types);

out:
	return ret;

}
