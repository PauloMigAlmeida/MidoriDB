/*
 * ast.c
 *
 *  Created on: 15/05/2023
 *      Author: paulo
 */

#include <tests/parser.h>
#include <parser/syntax.h>
#include <parser/ast.h>
#include <datastructure/linkedlist.h>

static void print_queue(struct queue *ct)
{
	for (size_t i = 0; i < queue_length(ct); i++) {
		printf("stack pos: %lu, content: %s\n", i, (char*)queue_peek_pos(ct, i));
	}
	printf("\n");
}

static void build_stack(char *stmt, struct queue *out)
{
	CU_ASSERT(queue_init(out));
	CU_ASSERT_EQUAL(syntax_parse(stmt, out), 0);
	printf("\n\n");
	print_queue(out);
}

static void create_table_case_1(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_create_node *create_node;
	struct list_head *pos = NULL;
	struct ast_column_def_node *entry = NULL;
	int i = 0;

	build_stack("CREATE TABLE IF NOT EXISTS A ("
			"f1 INTEGER PRIMARY KEY AUTO_INCREMENT,"
			"f2 INT UNIQUE,"
			"f3 DOUBLE NOT NULL);",
			&ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	create_node = (typeof(create_node))root;
	CU_ASSERT_EQUAL(create_node->node_type, AST_TYPE_CREATE);
	CU_ASSERT(create_node->if_not_exists);
	CU_ASSERT_EQUAL(list_length(create_node->node_children_head), 3);
	CU_ASSERT_STRING_EQUAL(create_node->table_name, "A");
	CU_ASSERT(list_is_empty(&create_node->head));
	CU_ASSERT_PTR_NOT_NULL(create_node->node_children_head);
	CU_ASSERT_FALSE(list_is_empty(create_node->node_children_head));

	list_for_each(pos, create_node->node_children_head)
	{
		entry = list_entry(pos, typeof(*entry), head);
		CU_ASSERT_EQUAL(entry->node_type, AST_TYPE_COLUMNDEF);
		CU_ASSERT_PTR_NULL(entry->node_children_head);
		CU_ASSERT_FALSE(list_is_empty(&entry->head));

		if (i == 0) {
			CU_ASSERT_STRING_EQUAL(entry->name, "f1");
			CU_ASSERT_EQUAL(entry->type, CT_INTEGER);
			CU_ASSERT_EQUAL(entry->precision, sizeof(uint64_t));
			CU_ASSERT(entry->attr_uniq_key);
			CU_ASSERT(entry->attr_auto_inc);
			CU_ASSERT(entry->attr_not_null);
			CU_ASSERT_FALSE(entry->attr_null);
			CU_ASSERT(entry->attr_prim_key);
		} else if (i == 1) {
			CU_ASSERT_STRING_EQUAL(entry->name, "f2");
			CU_ASSERT_EQUAL(entry->type, CT_INTEGER);
			CU_ASSERT_EQUAL(entry->precision, sizeof(uint64_t));
			CU_ASSERT(entry->attr_uniq_key);
			CU_ASSERT_FALSE(entry->attr_auto_inc);
			CU_ASSERT_FALSE(entry->attr_not_null);
			/* although it's UNIQUE, NULL means the absence of value and has a special treatment */
			CU_ASSERT(entry->attr_null);
			CU_ASSERT_FALSE(entry->attr_prim_key);
		} else {
			CU_ASSERT_STRING_EQUAL(entry->name, "f3");
			CU_ASSERT_EQUAL(entry->type, CT_DOUBLE);
			CU_ASSERT_EQUAL(entry->precision, sizeof(uint64_t));
			CU_ASSERT_FALSE(entry->attr_uniq_key);
			CU_ASSERT_FALSE(entry->attr_auto_inc);
			CU_ASSERT(entry->attr_not_null);
			CU_ASSERT_FALSE(entry->attr_null);
			CU_ASSERT_FALSE(entry->attr_prim_key);
		}
		i++;
	}

	queue_free(&ct);
	ast_free(root);

}

static void create_table_case_2(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_create_node *create_node;
	struct list_head *pos = NULL;
	struct ast_column_def_node *entry = NULL;
	int i = 0;

	build_stack("CREATE TABLE B ("
			"f1 INTEGER PRIMARY KEY AUTO_INCREMENT,"
			"f2 DOUBLE NOT NULL);",
			&ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	create_node = (typeof(create_node))root;
	CU_ASSERT_EQUAL(create_node->node_type, AST_TYPE_CREATE);
	CU_ASSERT_FALSE(create_node->if_not_exists);
	CU_ASSERT_EQUAL(list_length(create_node->node_children_head), 2);
	CU_ASSERT_STRING_EQUAL(create_node->table_name, "B");
	CU_ASSERT(list_is_empty(&create_node->head));
	CU_ASSERT_PTR_NOT_NULL(create_node->node_children_head);
	CU_ASSERT_FALSE(list_is_empty(create_node->node_children_head));

	list_for_each(pos, create_node->node_children_head)
	{
		entry = list_entry(pos, typeof(*entry), head);
		CU_ASSERT_EQUAL(entry->node_type, AST_TYPE_COLUMNDEF);
		CU_ASSERT_PTR_NULL(entry->node_children_head);
		CU_ASSERT_FALSE(list_is_empty(&entry->head));

		if (i == 0) {
			CU_ASSERT_STRING_EQUAL(entry->name, "f1");
			CU_ASSERT_EQUAL(entry->type, CT_INTEGER);
			CU_ASSERT_EQUAL(entry->precision, sizeof(uint64_t));
			CU_ASSERT(entry->attr_uniq_key);
			CU_ASSERT(entry->attr_auto_inc);
			CU_ASSERT(entry->attr_not_null);
			CU_ASSERT_FALSE(entry->attr_null);
			CU_ASSERT(entry->attr_prim_key);
		} else {
			CU_ASSERT_STRING_EQUAL(entry->name, "f2");
			CU_ASSERT_EQUAL(entry->type, CT_DOUBLE);
			CU_ASSERT_EQUAL(entry->precision, sizeof(uint64_t));
			CU_ASSERT_FALSE(entry->attr_uniq_key);
			CU_ASSERT_FALSE(entry->attr_auto_inc);
			CU_ASSERT(entry->attr_not_null);
			CU_ASSERT_FALSE(entry->attr_null);
			CU_ASSERT_FALSE(entry->attr_prim_key);
		}
		i++;
	}

	queue_free(&ct);
	ast_free(root);

}

static void create_table_case_3(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_node *entry = NULL;
	struct ast_create_node *create_node;
	struct ast_column_def_node *col_def_node;
	struct ast_index_def_node *idx_def_node;
	struct ast_index_column_node *idx_col_node;
	struct list_head *pos1 = NULL, *pos2 = NULL;
	int i = 0;
	int j = 0;

	build_stack("CREATE TABLE B ("
			"f1 INTEGER NOT NULL,"
			"f2 DOUBLE NOT NULL,"
			"PRIMARY KEY (f1,f2)"
			");",
			&ct);

	/**
	 * Notes to myself:
	 *
	 * The primary key definition in this case takes up 1 child node
	 *
	 * stack pos: 10, content: STMT
	 * stack pos: 9, content: CREATE 0 3 B
	 * stack pos: 8, content: PRIKEY 2
	 * stack pos: 7, content: COLUMN f2
	 * stack pos: 6, content: COLUMN f1
	 * stack pos: 5, content: COLUMNDEF 80000 f2
	 * stack pos: 4, content: ATTR NOTNULL
	 * stack pos: 3, content: STARTCOL
	 * stack pos: 2, content: COLUMNDEF 50000 f1
	 * stack pos: 1, content: ATTR NOTNULL
	 * stack pos: 0, content: STARTCOL
	 *
	 * We have to remember to differentiate them when going through the execution plan
	 *  by looking the node type up (always) :-)
	 */

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	create_node = (typeof(create_node))root;
	CU_ASSERT_EQUAL(create_node->node_type, AST_TYPE_CREATE);
	CU_ASSERT_FALSE(create_node->if_not_exists);
	CU_ASSERT_EQUAL(list_length(create_node->node_children_head), 3);
	CU_ASSERT_STRING_EQUAL(create_node->table_name, "B");
	CU_ASSERT(list_is_empty(&create_node->head));
	CU_ASSERT_PTR_NOT_NULL(create_node->node_children_head);
	CU_ASSERT_FALSE(list_is_empty(create_node->node_children_head));

	list_for_each(pos1, create_node->node_children_head)
	{
		entry = list_entry(pos1, typeof(*entry), head);
		CU_ASSERT_FALSE(list_is_empty(&entry->head));

		if (i == 0) {
			col_def_node = (struct ast_column_def_node*)entry;
			CU_ASSERT_EQUAL(col_def_node->node_type, AST_TYPE_COLUMNDEF);
			CU_ASSERT_PTR_NULL(col_def_node->node_children_head);
			CU_ASSERT_STRING_EQUAL(col_def_node->name, "f1");
			CU_ASSERT_EQUAL(col_def_node->type, CT_INTEGER);
			CU_ASSERT_EQUAL(col_def_node->precision, sizeof(uint64_t));
			CU_ASSERT_FALSE(col_def_node->attr_uniq_key);
			CU_ASSERT_FALSE(col_def_node->attr_auto_inc);
			CU_ASSERT(col_def_node->attr_not_null);
			CU_ASSERT_FALSE(col_def_node->attr_null);
			CU_ASSERT_FALSE(col_def_node->attr_prim_key);
		} else if (i == 1) {
			col_def_node = (struct ast_column_def_node*)entry;
			CU_ASSERT_EQUAL(col_def_node->node_type, AST_TYPE_COLUMNDEF);
			CU_ASSERT_PTR_NULL(col_def_node->node_children_head);
			CU_ASSERT_STRING_EQUAL(col_def_node->name, "f2");
			CU_ASSERT_EQUAL(col_def_node->type, CT_DOUBLE);
			CU_ASSERT_EQUAL(col_def_node->precision, sizeof(uint64_t));
			CU_ASSERT_FALSE(col_def_node->attr_uniq_key);
			CU_ASSERT_FALSE(col_def_node->attr_auto_inc);
			CU_ASSERT(col_def_node->attr_not_null);
			CU_ASSERT_FALSE(col_def_node->attr_null);
			CU_ASSERT_FALSE(col_def_node->attr_prim_key);
		} else {
			idx_def_node = (struct ast_index_def_node*)entry;
			CU_ASSERT_EQUAL(idx_def_node->node_type, AST_TYPE_INDEXDEF);
			CU_ASSERT_PTR_NOT_NULL(idx_def_node->node_children_head);
			CU_ASSERT_EQUAL(list_length(idx_def_node->node_children_head), 2);
			CU_ASSERT(idx_def_node->is_pk);
			CU_ASSERT_FALSE(idx_def_node->is_index);

			list_for_each(pos2, idx_def_node->node_children_head)
			{
				idx_col_node = list_entry(pos2, typeof(*idx_col_node), head);
				CU_ASSERT_EQUAL(idx_col_node->node_type, AST_TYPE_INDEXCOL);
				CU_ASSERT_PTR_NULL(idx_col_node->node_children_head);
				if (j == 0) {
					CU_ASSERT_STRING_EQUAL(idx_col_node->name, "f1");
				} else {
					CU_ASSERT_STRING_EQUAL(idx_col_node->name, "f2");
				}
				j++;
			}
		}
		i++;
	}

	queue_free(&ct);
	ast_free(root);

}

void test_ast_build_tree(void)
{

	/* create if not exists */
	create_table_case_1();
	/* create - no meta info*/
	create_table_case_2();
	/* create - primary key set after columns instead of column attribute */
	create_table_case_3();
	/**
	 *
	 * Parser -> Syntax -> Semantic -> Optimiser -> Execution Plan (This works for Select, Insert, Update, Delete)
	 * Parser -> Syntax -> Semantic -> Execution Plan (this works for CREATE, DROP?)
	 *
	 * Things to fix: 30/05/2023
	 *
	 * - tweak ast_build_tree so it can parse multiple STMTs in the future.
	 * - make ast_build_tree routine better... this looks hacky as fuck
	 */

}