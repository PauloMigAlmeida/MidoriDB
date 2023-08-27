/*
 * ast_insert.c
 *
 *  Created on: 22/08/2023
 *      Author: paulo
 */
#include <tests/parser.h>
#include <parser/syntax.h>
#include <parser/ast.h>
#include <datastructure/linkedlist.h>

static void delete_case_1(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_del_deleteone_node *delete_node;

	parse_stmt("DELETE FROM A;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	delete_node = (typeof(delete_node))root;
	CU_ASSERT_EQUAL(delete_node->node_type, AST_TYPE_DEL_DELETEONE);
	CU_ASSERT_EQUAL(list_length(delete_node->node_children_head), 0);
	CU_ASSERT_STRING_EQUAL(delete_node->table_name, "A");
	CU_ASSERT(list_is_empty(&delete_node->head));

	queue_free(&ct);
	ast_free(root);
}

static void delete_case_2(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_del_deleteone_node *delete_node;
	struct ast_del_cmp_node *cmp_node;
	struct ast_del_exprval_node *val_node;
	struct list_head *pos1, *pos2;
	int i = 0;

	parse_stmt("DELETE FROM A WHERE id = 1;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	delete_node = (typeof(delete_node))root;
	CU_ASSERT_EQUAL(delete_node->node_type, AST_TYPE_DEL_DELETEONE);
	CU_ASSERT_EQUAL(list_length(delete_node->node_children_head), 1);
	CU_ASSERT_STRING_EQUAL(delete_node->table_name, "A");
	CU_ASSERT(list_is_empty(&delete_node->head));

	list_for_each(pos1, delete_node->node_children_head)
	{
		cmp_node = list_entry(pos1, typeof(*cmp_node), head);
		CU_ASSERT_EQUAL(cmp_node->node_type, AST_TYPE_DEL_CMP);
		CU_ASSERT_EQUAL(cmp_node->cmp_type, AST_CMP_EQUALS_OP);
		CU_ASSERT_EQUAL(list_length(cmp_node->node_children_head), 2);
		CU_ASSERT_FALSE(list_is_empty(&cmp_node->head));

		list_for_each(pos2, cmp_node->node_children_head)
		{
			val_node = list_entry(pos2, typeof(*val_node), head);

			CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_DEL_EXPRVAL);
			CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);
			CU_ASSERT_FALSE(list_is_empty(&val_node->head));

			if (i == 0) {
				CU_ASSERT(val_node->value_type.is_intnum);
				CU_ASSERT_EQUAL(val_node->int_val, 1);
			} else {
				CU_ASSERT(val_node->value_type.is_name);
				CU_ASSERT_STRING_EQUAL(val_node->name_val, "id");
			}

			i++;
		}
	}

	queue_free(&ct);
	ast_free(root);
}

void test_ast_build_tree_delete(void)
{
	/* no where-clause */
	delete_case_1();

	/* where-clause; single condition */
	delete_case_2();

}
