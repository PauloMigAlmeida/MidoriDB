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
				CU_ASSERT(val_node->value_type.is_name);
				CU_ASSERT_STRING_EQUAL(val_node->name_val, "id");
			} else {
				CU_ASSERT(val_node->value_type.is_intnum);
				CU_ASSERT_EQUAL(val_node->int_val, 1);
			}

			i++;
		}
	}

	queue_free(&ct);
	ast_free(root);
}

static void delete_case_3(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_del_deleteone_node *delete_node;
	struct ast_del_cmp_node *cmp_node;
	struct ast_del_exprval_node *val_node;
	struct list_head *pos1, *pos2;
	int i = 0;

	parse_stmt("DELETE FROM A WHERE 1 = id;", &ct);

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

static void delete_case_4(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_del_deleteone_node *delete_node;
	struct ast_del_logop_node *logop_node_1, *logop_node_2;
	struct ast_del_cmp_node *cmp_node_1, *cmp_node_2;
	struct ast_del_exprval_node *val_node_1, *val_node_2;
	struct list_head *pos1, *pos2, *pos3, *pos4, *pos5;
	int i = 0, j = 0, k = 0;

	parse_stmt("DELETE FROM A WHERE id = 1 OR id = 2 OR id = 3;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	delete_node = (typeof(delete_node))root;
	CU_ASSERT_EQUAL(delete_node->node_type, AST_TYPE_DEL_DELETEONE);
	CU_ASSERT_EQUAL(list_length(delete_node->node_children_head), 1);
	CU_ASSERT_STRING_EQUAL(delete_node->table_name, "A");
	CU_ASSERT(list_is_empty(&delete_node->head));

	list_for_each(pos1, delete_node->node_children_head)
	{
		logop_node_1 = list_entry(pos1, typeof(*logop_node_1), head);
		CU_ASSERT_EQUAL(logop_node_1->node_type, AST_TYPE_DEL_LOGOP);
		CU_ASSERT_EQUAL(logop_node_1->logop_type, AST_LOGOP_TYPE_OR);
		CU_ASSERT_EQUAL(list_length(logop_node_1->node_children_head), 2);
		CU_ASSERT_FALSE(list_is_empty(&logop_node_1->head));

		list_for_each(pos2, logop_node_1->node_children_head)
		{
			if (i == 0) {
				logop_node_2 = list_entry(pos2, typeof(*logop_node_2), head);
				CU_ASSERT_EQUAL(logop_node_2->node_type, AST_TYPE_DEL_LOGOP);
				CU_ASSERT_EQUAL(logop_node_2->logop_type, AST_LOGOP_TYPE_OR);
				CU_ASSERT_EQUAL(list_length(logop_node_2->node_children_head), 2);
				CU_ASSERT_FALSE(list_is_empty(&logop_node_2->head));

				list_for_each(pos3, logop_node_2->node_children_head)
				{
					cmp_node_1 = list_entry(pos3, typeof(*cmp_node_1), head);
					CU_ASSERT_EQUAL(cmp_node_1->node_type, AST_TYPE_DEL_CMP);
					CU_ASSERT_EQUAL(cmp_node_1->cmp_type, AST_CMP_EQUALS_OP);
					CU_ASSERT_EQUAL(list_length(cmp_node_1->node_children_head), 2);
					CU_ASSERT_FALSE(list_is_empty(&cmp_node_1->head));

					list_for_each(pos5, cmp_node_1->node_children_head)
					{
						val_node_1 = list_entry(pos5, typeof(*val_node_1), head);

						CU_ASSERT_EQUAL(val_node_1->node_type, AST_TYPE_DEL_EXPRVAL);
						CU_ASSERT_EQUAL(list_length(val_node_1->node_children_head), 0);
						CU_ASSERT_FALSE(list_is_empty(&val_node_1->head));

						if (j == 0) {
							CU_ASSERT(val_node_1->value_type.is_name);
							CU_ASSERT_STRING_EQUAL(val_node_1->name_val, "id");
						} else if (j == 1) {
							CU_ASSERT(val_node_1->value_type.is_intnum);
							CU_ASSERT_EQUAL(val_node_1->int_val, 1);
						} else if (j == 2) {
							CU_ASSERT(val_node_1->value_type.is_name);
							CU_ASSERT_STRING_EQUAL(val_node_1->name_val, "id");
						} else if (j == 3) {
							CU_ASSERT(val_node_1->value_type.is_intnum);
							CU_ASSERT_EQUAL(val_node_1->int_val, 2);
						}

						j++;
					}
				}

			} else {
				cmp_node_2 = list_entry(pos2, typeof(*cmp_node_2), head);
				CU_ASSERT_EQUAL(cmp_node_2->node_type, AST_TYPE_DEL_CMP);
				CU_ASSERT_EQUAL(cmp_node_2->cmp_type, AST_CMP_EQUALS_OP);
				CU_ASSERT_EQUAL(list_length(cmp_node_2->node_children_head), 2);
				CU_ASSERT_FALSE(list_is_empty(&cmp_node_2->head));

				list_for_each(pos4, cmp_node_2->node_children_head)
				{
					val_node_2 = list_entry(pos4, typeof(*val_node_2), head);

					CU_ASSERT_EQUAL(val_node_2->node_type, AST_TYPE_DEL_EXPRVAL);
					CU_ASSERT_EQUAL(list_length(val_node_2->node_children_head), 0);
					CU_ASSERT_FALSE(list_is_empty(&val_node_2->head));

					if (k == 0) {
						CU_ASSERT(val_node_2->value_type.is_name);
						CU_ASSERT_STRING_EQUAL(val_node_2->name_val, "id");
					} else {
						CU_ASSERT(val_node_2->value_type.is_intnum);
						CU_ASSERT_EQUAL(val_node_2->int_val, 3);
					}

					k++;
				}
			}
			i++;
		}
	}

	queue_free(&ct);
	ast_free(root);
}

static void delete_case_5(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_del_deleteone_node *delete_node;
	struct ast_del_isxin_node *isxin_node;
	struct ast_del_exprval_node *val_node;
	struct list_head *pos1, *pos2;
	int i = 0;

	parse_stmt("DELETE FROM A WHERE id IN (1,2);", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	delete_node = (typeof(delete_node))root;
	CU_ASSERT_EQUAL(delete_node->node_type, AST_TYPE_DEL_DELETEONE);
	CU_ASSERT_EQUAL(list_length(delete_node->node_children_head), 1);
	CU_ASSERT_STRING_EQUAL(delete_node->table_name, "A");
	CU_ASSERT(list_is_empty(&delete_node->head));

	list_for_each(pos1, delete_node->node_children_head)
	{
		isxin_node = list_entry(pos1, typeof(*isxin_node), head);
		CU_ASSERT_EQUAL(isxin_node->node_type, AST_TYPE_DEL_EXPRISXIN);
		CU_ASSERT_FALSE(isxin_node->is_negation);
		CU_ASSERT_EQUAL(list_length(isxin_node->node_children_head), 3);
		CU_ASSERT_FALSE(list_is_empty(&isxin_node->head));

		list_for_each(pos2, isxin_node->node_children_head)
		{
			val_node = list_entry(pos2, typeof(*val_node), head);
			CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_DEL_EXPRVAL);
			CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);
			CU_ASSERT_FALSE(list_is_empty(&val_node->head));

			if (i == 0) {
				CU_ASSERT(val_node->value_type.is_name);
				CU_ASSERT_STRING_EQUAL(val_node->name_val, "id");
			} else if (i == 1) {
				CU_ASSERT(val_node->value_type.is_intnum);
				CU_ASSERT_EQUAL(val_node->int_val, 1);
			} else {
				CU_ASSERT(val_node->value_type.is_intnum);
				CU_ASSERT_EQUAL(val_node->int_val, 2);
			}
			i++;
		}

	}

	queue_free(&ct);
	ast_free(root);
}

static void delete_case_6(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_del_deleteone_node *delete_node;
	struct ast_del_isxin_node *isxin_node;
	struct ast_del_exprval_node *val_node;
	struct list_head *pos1, *pos2;
	int i = 0;

	parse_stmt("DELETE FROM A WHERE id NOT IN (1,2);", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	delete_node = (typeof(delete_node))root;
	CU_ASSERT_EQUAL(delete_node->node_type, AST_TYPE_DEL_DELETEONE);
	CU_ASSERT_EQUAL(list_length(delete_node->node_children_head), 1);
	CU_ASSERT_STRING_EQUAL(delete_node->table_name, "A");
	CU_ASSERT(list_is_empty(&delete_node->head));

	list_for_each(pos1, delete_node->node_children_head)
	{
		isxin_node = list_entry(pos1, typeof(*isxin_node), head);
		CU_ASSERT_EQUAL(isxin_node->node_type, AST_TYPE_DEL_EXPRISXIN);
		CU_ASSERT(isxin_node->is_negation);
		CU_ASSERT_EQUAL(list_length(isxin_node->node_children_head), 3);
		CU_ASSERT_FALSE(list_is_empty(&isxin_node->head));

		list_for_each(pos2, isxin_node->node_children_head)
		{
			val_node = list_entry(pos2, typeof(*val_node), head);
			CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_DEL_EXPRVAL);
			CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);
			CU_ASSERT_FALSE(list_is_empty(&val_node->head));

			if (i == 0) {
				CU_ASSERT(val_node->value_type.is_name);
				CU_ASSERT_STRING_EQUAL(val_node->name_val, "id");
			} else if (i == 1) {
				CU_ASSERT(val_node->value_type.is_intnum);
				CU_ASSERT_EQUAL(val_node->int_val, 1);
			} else {
				CU_ASSERT(val_node->value_type.is_intnum);
				CU_ASSERT_EQUAL(val_node->int_val, 2);
			}
			i++;
		}

	}

	queue_free(&ct);
	ast_free(root);
}

static void delete_case_7(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_del_deleteone_node *delete_node;
	struct ast_del_isxnull_node *isxnull;
	struct ast_del_exprval_node *val_node;
	struct list_head *pos1, *pos2;

	parse_stmt("DELETE FROM A WHERE dob IS NULL;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	delete_node = (typeof(delete_node))root;
	CU_ASSERT_EQUAL(delete_node->node_type, AST_TYPE_DEL_DELETEONE);
	CU_ASSERT_EQUAL(list_length(delete_node->node_children_head), 1);
	CU_ASSERT_STRING_EQUAL(delete_node->table_name, "A");
	CU_ASSERT(list_is_empty(&delete_node->head));

	list_for_each(pos1, delete_node->node_children_head)
	{
		isxnull = list_entry(pos1, typeof(*isxnull), head);
		CU_ASSERT_EQUAL(isxnull->node_type, AST_TYPE_DEL_EXPRISXNULL);
		CU_ASSERT_FALSE(isxnull->is_negation);
		CU_ASSERT_EQUAL(list_length(isxnull->node_children_head), 1);
		CU_ASSERT_FALSE(list_is_empty(&isxnull->head));

		list_for_each(pos2, isxnull->node_children_head)
		{
			val_node = list_entry(pos2, typeof(*val_node), head);
			CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_DEL_EXPRVAL);
			CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);
			CU_ASSERT_FALSE(list_is_empty(&val_node->head));

			CU_ASSERT(val_node->value_type.is_name);
			CU_ASSERT_STRING_EQUAL(val_node->name_val, "dob");
		}

	}

	queue_free(&ct);
	ast_free(root);
}

static void delete_case_8(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_del_deleteone_node *delete_node;
	struct ast_del_isxnull_node *isxnull_node;
	struct ast_del_exprval_node *val_node;
	struct list_head *pos1, *pos2;

	parse_stmt("DELETE FROM A WHERE dob IS NOT NULL;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	delete_node = (typeof(delete_node))root;
	CU_ASSERT_EQUAL(delete_node->node_type, AST_TYPE_DEL_DELETEONE);
	CU_ASSERT_EQUAL(list_length(delete_node->node_children_head), 1);
	CU_ASSERT_STRING_EQUAL(delete_node->table_name, "A");
	CU_ASSERT(list_is_empty(&delete_node->head));

	list_for_each(pos1, delete_node->node_children_head)
	{
		isxnull_node = list_entry(pos1, typeof(*isxnull_node), head);
		CU_ASSERT_EQUAL(isxnull_node->node_type, AST_TYPE_DEL_EXPRISXNULL);
		CU_ASSERT(isxnull_node->is_negation);
		CU_ASSERT_EQUAL(list_length(isxnull_node->node_children_head), 1);
		CU_ASSERT_FALSE(list_is_empty(&isxnull_node->head));

		list_for_each(pos2, isxnull_node->node_children_head)
		{
			val_node = list_entry(pos2, typeof(*val_node), head);
			CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_DEL_EXPRVAL);
			CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);
			CU_ASSERT_FALSE(list_is_empty(&val_node->head));

			CU_ASSERT(val_node->value_type.is_name);
			CU_ASSERT_STRING_EQUAL(val_node->name_val, "dob");
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

	/* where-clause; single condition; inverted order (id=1 -> 1=id) */
	delete_case_3();

	/* where-clause; multiple condition; */
	delete_case_4();

	/* where-clause; in-condition; */
	delete_case_5();

	/* where-clause; in-condition; */
	delete_case_6();

	/* where-clause; is null */
	delete_case_7();

	/* where-clause; is not null */
	delete_case_8();

}
