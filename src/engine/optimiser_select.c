/*
 * optimiser_select.c
 *
 *  Created on: 14/11/2023
 *      Author: paulo
 */

#include <engine/optimiser.h>

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

static int build_alias_hashtable(struct ast_node *root, struct query_output *output, struct hashtable *out_tbl_alias,
		struct hashtable *out_col_alias)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_alias_node *alias_node;
	struct ast_sel_table_node *table_node;

	struct hashtable *tmp_ht;
	char *key;
	char *value;
	int ret;

	if (root->node_type == AST_TYPE_SEL_ALIAS) {
		alias_node = (typeof(alias_node))root;

		list_for_each(pos, alias_node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (tmp_entry->node_type == AST_TYPE_SEL_TABLE) {
				table_node = (typeof(table_node))tmp_entry;
				tmp_ht = out_tbl_alias;
				key = alias_node->alias_value;
				value = table_node->table_name;
			} else {
				// column alias
				tmp_ht = out_col_alias;
				key = alias_node->alias_value;
				value = alias_node->alias_value;
			}

			if (!hashtable_put(tmp_ht, key, strlen(key) + 1, value, strlen(value) + 1)) {
				snprintf(output->error.message, sizeof(output->error.message),
						"optimiser phase: internal error\n");
				return -MIDORIDB_INTERNAL;
			}
		}
	} else {
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if ((ret = build_alias_hashtable(tmp_entry, output, out_tbl_alias, out_col_alias)))
				return ret;
		}
	}

	return MIDORIDB_OK;
}

static bool column_exists(struct table *table, char *col_name)
{
	for (int i = 0; i < table->column_count; i++) {
		if (strcmp(table->columns[i].name, col_name) == 0)
			return true;
	}

	return false;
}

static struct table* table_with_column_name(struct database *db, struct ast_node *root, char *col_name)
{
	struct list_head *pos;
	struct ast_node *tmp_entry;
	struct ast_sel_table_node *table_node;
	struct table *table;

	if (root->node_type == AST_TYPE_SEL_TABLE) {
		table_node = (typeof(table_node))root;
		table = database_table_get(db, table_node->table_name);
		if (column_exists(table, col_name))
			return table;
	} else {
		list_for_each(pos, root->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);
			if ((table = table_with_column_name(db, tmp_entry, col_name))) {
				return table;
			}
		}
	}

	return NULL;
}

/* sanity checks during compilation to ensure logic below holds up */
BUILD_BUG(MEMBER_SIZE(struct ast_sel_fieldname_node, col_name) == MEMBER_SIZE(struct ast_sel_exprval_node, name_val),
		"must have the same size");
BUILD_BUG(MEMBER_SIZE(struct ast_sel_fieldname_node, table_name) == MEMBER_SIZE(struct table, name),
		"must have the same size");

static int replace_expr_name(struct database *db, struct ast_node *node, struct ast_node *root,
		struct query_output *output, struct hashtable *out_col_alias)
{
	struct ast_sel_exprval_node *val_node;
	struct ast_sel_fieldname_node *new_field;
	struct table *table;

	val_node = (typeof(val_node))node;

	if (val_node->value_type.is_name) {

		/* Check if that's a column alias, if it isn't then we can replace it with a fqfield */
		if (!hashtable_get(out_col_alias, val_node->name_val, strlen(val_node->name_val) + 1)) {
			table = table_with_column_name(db, root, val_node->name_val);

			/* init new fieldname node */
			if (!(new_field = zalloc(sizeof(*new_field))))
				goto err;

			new_field->node_type = AST_TYPE_SEL_FIELDNAME;

			if (!(new_field->node_children_head = malloc(sizeof(*new_field->node_children_head))))
				goto err_head;

			list_head_init(&new_field->head);
			list_head_init(new_field->node_children_head);

			/* boundaries guaranteed via BUILD_BUG above */
			strncpy(new_field->table_name, table->name, sizeof(new_field->table_name));
			strncpy(new_field->col_name, val_node->name_val, sizeof(new_field->col_name));

			/* replace node within the tree */
			list_add(&new_field->head, val_node->head.prev);
			list_del(&val_node->head);

			/* free old node */
			ast_free((struct ast_node*)val_node);
		}
	}

	return MIDORIDB_OK;

err_head:
	free(new_field);
err:
	snprintf(output->error.message, sizeof(output->error.message), "optimiser phase: internal error\n");
	return -MIDORIDB_NOMEM;
}

static int replace_fieldname(struct database *db, struct ast_node *node, struct hashtable *out_tbl_alias)
{
	struct ast_sel_fieldname_node *field_node;
	struct hashtable_value *lookup;

	field_node = (typeof(field_node))node;

	/* is this a table alias? */
	lookup = hashtable_get(out_tbl_alias, field_node->table_name, strlen(field_node->table_name) + 1);
	if (lookup) {
		/* boundaries guaranteed via BUILD_BUG above */
		strncpy(field_node->table_name, (char*)lookup->content, sizeof(field_node->table_name));
	} else if (database_table_exists(db, field_node->table_name)) {
		/* user specified the fqfield - silly but still valid.. so do nothing  */
	} else {
		/* is this something that I haven't foreseen? */
		BUG_GENERIC();
	}

	return MIDORIDB_OK;
}

static int replace_selectall(struct database *db, struct ast_node *node, struct ast_node *root, struct query_output *output)
{
	struct list_head *pos, *tmp_pos;
	struct ast_node *tmp_entry;
	struct ast_sel_table_node *table_node;
	struct ast_sel_fieldname_node *new_field;
	struct table *table;
	struct column *column;
	int ret;

	if (node->node_type == AST_TYPE_SEL_TABLE) {
		table_node = (typeof(table_node))node;
		table = database_table_get(db, table_node->table_name);

		for (int i = table->column_count - 1; i >= 0; i--) {
			column = &table->columns[i];

			/* init new fieldname node */
			if (!(new_field = zalloc(sizeof(*new_field))))
				goto err;

			new_field->node_type = AST_TYPE_SEL_FIELDNAME;

			if (!(new_field->node_children_head = malloc(sizeof(*new_field->node_children_head))))
				goto err_head;

			list_head_init(&new_field->head);
			list_head_init(new_field->node_children_head);

			strncpy(new_field->table_name, table->name, sizeof(new_field->table_name));
			strncpy(new_field->col_name, column->name, sizeof(new_field->col_name));

			/* add it to the tree - adjacently to the SELECTALL node for now */
			list_add(&new_field->head, root->node_children_head);
		}

	}

	list_for_each_safe(pos, tmp_pos, node->node_children_head)
	{
		tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

		if ((ret = replace_selectall(db, tmp_entry, root, output)))
			return ret;
	}

	return MIDORIDB_OK;

err_head:
	free(new_field);
err:
	snprintf(output->error.message, sizeof(output->error.message), "optimiser phase: internal error\n");
	return -MIDORIDB_NOMEM;
}

static int replace_simple_expr(struct database *db, struct ast_node *node, struct ast_node *root,
		struct query_output *output, struct hashtable *out_tbl_alias, struct hashtable *out_col_alias)
{
	struct list_head *pos, *tmp_pos;
	struct ast_node *tmp_entry;
	int ret;

	if (node->node_type == AST_TYPE_SEL_EXPRVAL) {
		if ((ret = replace_expr_name(db, node, root, output, out_col_alias)))
			return ret;
	} else if (node->node_type == AST_TYPE_SEL_FIELDNAME) {
		if ((ret = replace_fieldname(db, node, out_tbl_alias)))
			return ret;
	} else if (node->node_type == AST_TYPE_SEL_SELECTALL) {
		if ((ret = replace_selectall(db, root, root, output)))
			return ret;

		/* delete selectall node */
		list_del(&node->head);
		ast_free(node);
	} else {
		list_for_each_safe(pos, tmp_pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if ((ret = replace_simple_expr(db, tmp_entry, root, output, out_tbl_alias, out_col_alias)))
				return ret;
		}
	}

	return MIDORIDB_OK;
}

int remove_table_aliases(struct ast_node *node, struct query_output *output)
{
	struct ast_node *tmp_entry;
	struct list_head *pos, *tmp_pos;
	struct ast_sel_table_node *table_node;
	int ret;

	if (node->node_type == AST_TYPE_SEL_ALIAS) {
		list_for_each(pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if (tmp_entry->node_type == AST_TYPE_SEL_TABLE) {
				table_node = (typeof(table_node))tmp_entry;

				/* replace node within the tree */
				list_del(&table_node->head);
				list_add(&table_node->head, node->head.prev);
				list_del(&node->head);

				/* free old node */
				ast_free(node);

				/* we have to break free otherwise the linked-list iteration will mem-leak */
				break;
			}
		}

	} else {
		list_for_each_safe(pos, tmp_pos, node->node_children_head)
		{
			tmp_entry = list_entry(pos, typeof(*tmp_entry), head);

			if ((ret = remove_table_aliases(tmp_entry, output)))
				return ret;
		}
	}

	return MIDORIDB_OK;
}

int optimiser_run_select_stmt(struct database *db, struct ast_sel_select_node *node, struct query_output *output)
{
	struct hashtable table_alias = {0};
	struct hashtable column_alias = {0};
	int ret = MIDORIDB_OK;

	if (!hashtable_init(&table_alias, &hashtable_str_compare, &hashtable_str_hash)) {
		snprintf(output->error.message, sizeof(output->error.message), "optimiser phase: internal error\n");
		ret = -MIDORIDB_INTERNAL;
		goto out;
	}

	if (!hashtable_init(&column_alias, &hashtable_str_compare, &hashtable_str_hash)) {
		snprintf(output->error.message, sizeof(output->error.message), "optimiser phase: internal error\n");
		ret = -MIDORIDB_INTERNAL;
		goto cleanup_col_ht;
	}

	/* build aliases hashtable */
	if ((ret = build_alias_hashtable((struct ast_node*)node, output, &table_alias, &column_alias)))
		goto cleanup_tbl_ht;

	/* Replace every exprval (name) with a fieldname using the full-qualified column format
	 * so that executor's code becomes "less" complicated. */
	if ((ret = replace_simple_expr(db, (struct ast_node*)node, (struct ast_node*)node, output, &table_alias,
					&column_alias)))
		goto cleanup_tbl_ht;

	/* remove table aliases as they are no longer useful */
	if ((ret = remove_table_aliases((struct ast_node*)node, output)))
		goto cleanup_tbl_ht;

cleanup_tbl_ht:
	hashtable_foreach(&column_alias, &free_hashmap_entries, NULL);
	hashtable_free(&column_alias);

cleanup_col_ht:
	hashtable_foreach(&table_alias, &free_hashmap_entries, NULL);
	hashtable_free(&table_alias);
out:
	return ret;

}
