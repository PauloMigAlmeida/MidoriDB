/*
 * ast.c
 *
 *  Created on: 15/05/2023
 *      Author: paulo
 */

#include <tests/parser.h>
#include <parser/syntax.h>
#include <parser/ast.h>

static void print_queue(struct queue *ct)
{
	for (size_t i = 0; i <= (ct->arr->len / sizeof(uintptr_t)); i++) {
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
	CU_ASSERT_EQUAL(create_node->column_count, 3);
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
	CU_ASSERT_EQUAL(create_node->column_count, 2);
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
	struct ast_create_node *create_node;
	struct list_head *pos = NULL;
	struct ast_column_def_node *entry = NULL;
	int i = 0;

	build_stack("CREATE TABLE B ("
			"f1 INTEGER NOT NULL,"
			"f2 DOUBLE NOT NULL,"
			"PRIMARY KEY (f1,f2)"
			");",
			&ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	create_node = (typeof(create_node))root;
	CU_ASSERT_EQUAL(create_node->node_type, AST_TYPE_CREATE);
	CU_ASSERT_FALSE(create_node->if_not_exists);
	CU_ASSERT_EQUAL(create_node->column_count, 2);
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

void test_ast_build_tree(void)
{

	/* create if not exists */
	create_table_case_1();
	/* create - no meta info*/
	create_table_case_2();
	/* create - primary key set after columns instead of column attribute */
//	create_table_case_3();
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
