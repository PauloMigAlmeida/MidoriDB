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

static void update_case_1(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_upd_update_node *update_node;
	struct ast_upd_assign_node *assign_node;
	struct ast_upd_exprval_node *val_node;
	struct list_head *pos1, *pos2;

	parse_stmt("UPDATE A SET id=42;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	update_node = (typeof(update_node))root;
	CU_ASSERT_EQUAL(update_node->node_type, AST_TYPE_UPD_UPDATE);
	CU_ASSERT_STRING_EQUAL(update_node->table_name, "A");
	CU_ASSERT(list_is_empty(&update_node->head));
	CU_ASSERT_EQUAL(list_length(update_node->node_children_head), 1);
	list_for_each(pos1, update_node->node_children_head)
	{
		assign_node = list_entry(pos1, typeof(*assign_node), head);
		CU_ASSERT_EQUAL(assign_node->node_type, AST_TYPE_UPD_ASSIGN);
		CU_ASSERT_STRING_EQUAL(assign_node->field_name, "id");
		CU_ASSERT_FALSE(list_is_empty(&assign_node->head));
		CU_ASSERT_EQUAL(list_length(assign_node->node_children_head), 1);

		list_for_each(pos2, assign_node->node_children_head)
		{
			val_node = list_entry(pos2, typeof(*val_node), head);
			CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_UPD_EXPRVAL);
			CU_ASSERT(val_node->value_type.is_intnum);
			CU_ASSERT_EQUAL(val_node->int_val, 42);
			CU_ASSERT_FALSE(list_is_empty(&val_node->head));
			CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);

		}
	}

	queue_free(&ct);
	ast_free(root);
}

static void update_case_2(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_upd_update_node *update_node;
	struct ast_upd_assign_node *assign_node;
	struct ast_upd_cmp_node *cmp_node;
	struct ast_upd_exprval_node *val_node;
	struct list_head *pos1, *pos2;
	int i = 0, j = 0;

	parse_stmt("UPDATE A SET id=42 WHERE id = 1;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	update_node = (typeof(update_node))root;
	CU_ASSERT_EQUAL(update_node->node_type, AST_TYPE_UPD_UPDATE);
	CU_ASSERT_EQUAL(list_length(update_node->node_children_head), 2);
	CU_ASSERT_STRING_EQUAL(update_node->table_name, "A");
	CU_ASSERT(list_is_empty(&update_node->head));

	list_for_each(pos1, update_node->node_children_head)
	{
		if (i == 0) {
			assign_node = list_entry(pos1, typeof(*assign_node), head);
			CU_ASSERT_EQUAL(assign_node->node_type, AST_TYPE_UPD_ASSIGN);
			CU_ASSERT_STRING_EQUAL(assign_node->field_name, "id");
			CU_ASSERT_FALSE(list_is_empty(&assign_node->head));
			CU_ASSERT_EQUAL(list_length(assign_node->node_children_head), 1);

			list_for_each(pos2, assign_node->node_children_head)
			{
				val_node = list_entry(pos2, typeof(*val_node), head);
				CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_UPD_EXPRVAL);
				CU_ASSERT(val_node->value_type.is_intnum);
				CU_ASSERT_EQUAL(val_node->int_val, 42);
				CU_ASSERT_FALSE(list_is_empty(&val_node->head));
				CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);

			}
		} else if (i == 1) {
			cmp_node = list_entry(pos1, typeof(*cmp_node), head);
			CU_ASSERT_EQUAL(cmp_node->node_type, AST_TYPE_UPD_CMP);
			CU_ASSERT_EQUAL(cmp_node->cmp_type, AST_CMP_EQUALS_OP);
			CU_ASSERT_EQUAL(list_length(cmp_node->node_children_head), 2);
			CU_ASSERT_FALSE(list_is_empty(&cmp_node->head));

			list_for_each(pos2, cmp_node->node_children_head)
			{
				val_node = list_entry(pos2, typeof(*val_node), head);

				CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_UPD_EXPRVAL);
				CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);
				CU_ASSERT_FALSE(list_is_empty(&val_node->head));

				if (j == 0) {
					CU_ASSERT(val_node->value_type.is_name);
					CU_ASSERT_STRING_EQUAL(val_node->name_val, "id");
				} else {
					CU_ASSERT(val_node->value_type.is_intnum);
					CU_ASSERT_EQUAL(val_node->int_val, 1);
				}

				j++;
			}
		}
		i++;
	}

	queue_free(&ct);
	ast_free(root);
}

static void update_case_3(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_upd_update_node *update_node;
	struct ast_upd_assign_node *assign_node;
	struct ast_upd_cmp_node *cmp_node;
	struct ast_upd_exprval_node *val_node;
	struct list_head *pos1, *pos2;
	int i = 0, j = 0;

	parse_stmt("UPDATE A SET id=42 WHERE 1 = id;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	update_node = (typeof(update_node))root;
	CU_ASSERT_EQUAL(update_node->node_type, AST_TYPE_UPD_UPDATE);
	CU_ASSERT_EQUAL(list_length(update_node->node_children_head), 2);
	CU_ASSERT_STRING_EQUAL(update_node->table_name, "A");
	CU_ASSERT(list_is_empty(&update_node->head));

	list_for_each(pos1, update_node->node_children_head)
	{
		if (i == 0) {
			assign_node = list_entry(pos1, typeof(*assign_node), head);
			CU_ASSERT_EQUAL(assign_node->node_type, AST_TYPE_UPD_ASSIGN);
			CU_ASSERT_STRING_EQUAL(assign_node->field_name, "id");
			CU_ASSERT_FALSE(list_is_empty(&assign_node->head));
			CU_ASSERT_EQUAL(list_length(assign_node->node_children_head), 1);

			list_for_each(pos2, assign_node->node_children_head)
			{
				val_node = list_entry(pos2, typeof(*val_node), head);
				CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_UPD_EXPRVAL);
				CU_ASSERT(val_node->value_type.is_intnum);
				CU_ASSERT_EQUAL(val_node->int_val, 42);
				CU_ASSERT_FALSE(list_is_empty(&val_node->head));
				CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);

			}
		} else if (i == 1) {
			cmp_node = list_entry(pos1, typeof(*cmp_node), head);
			CU_ASSERT_EQUAL(cmp_node->node_type, AST_TYPE_UPD_CMP);
			CU_ASSERT_EQUAL(cmp_node->cmp_type, AST_CMP_EQUALS_OP);
			CU_ASSERT_EQUAL(list_length(cmp_node->node_children_head), 2);
			CU_ASSERT_FALSE(list_is_empty(&cmp_node->head));

			list_for_each(pos2, cmp_node->node_children_head)
			{
				val_node = list_entry(pos2, typeof(*val_node), head);

				CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_UPD_EXPRVAL);
				CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);
				CU_ASSERT_FALSE(list_is_empty(&val_node->head));

				if (j == 0) {
					CU_ASSERT(val_node->value_type.is_intnum);
					CU_ASSERT_EQUAL(val_node->int_val, 1);
				} else {
					CU_ASSERT(val_node->value_type.is_name);
					CU_ASSERT_STRING_EQUAL(val_node->name_val, "id");
				}

				j++;
			}
		}
		i++;
	}

	queue_free(&ct);
	ast_free(root);
}

static void update_case_4(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_upd_update_node *update_node;
	struct ast_upd_assign_node *assign_node;
	struct ast_upd_logop_node *logop_node_1, *logop_node_2;
	struct ast_upd_cmp_node *cmp_node_1, *cmp_node_2;
	struct ast_upd_exprval_node *val_node_1, *val_node_2, *val_node_3;
	struct list_head *pos1, *pos2, *pos3, *pos4, *pos5;
	int i = 0, j = 0, k = 0, l = 0;

	parse_stmt("UPDATE A SET id = 42 WHERE id = 1 OR id = 2 OR id = 3;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	update_node = (typeof(update_node))root;
	CU_ASSERT_EQUAL(update_node->node_type, AST_TYPE_UPD_UPDATE);
	CU_ASSERT_EQUAL(list_length(update_node->node_children_head), 2);
	CU_ASSERT_STRING_EQUAL(update_node->table_name, "A");
	CU_ASSERT(list_is_empty(&update_node->head));

	list_for_each(pos1, update_node->node_children_head)
	{
		if (i == 0) {
			assign_node = list_entry(pos1, typeof(*assign_node), head);
			CU_ASSERT_EQUAL(assign_node->node_type, AST_TYPE_UPD_ASSIGN);
			CU_ASSERT_STRING_EQUAL(assign_node->field_name, "id");
			CU_ASSERT_FALSE(list_is_empty(&assign_node->head));
			CU_ASSERT_EQUAL(list_length(assign_node->node_children_head), 1);

			list_for_each(pos2, assign_node->node_children_head)
			{
				val_node_1 = list_entry(pos2, typeof(*val_node_1), head);
				CU_ASSERT_EQUAL(val_node_1->node_type, AST_TYPE_UPD_EXPRVAL);
				CU_ASSERT(val_node_1->value_type.is_intnum);
				CU_ASSERT_EQUAL(val_node_1->int_val, 42);
				CU_ASSERT_FALSE(list_is_empty(&val_node_1->head));
				CU_ASSERT_EQUAL(list_length(val_node_1->node_children_head), 0);

			}
		} else if (i == 1) {

			logop_node_1 = list_entry(pos1, typeof(*logop_node_1), head);
			CU_ASSERT_EQUAL(logop_node_1->node_type, AST_TYPE_UPD_LOGOP);
			CU_ASSERT_EQUAL(logop_node_1->logop_type, AST_LOGOP_TYPE_OR);
			CU_ASSERT_EQUAL(list_length(logop_node_1->node_children_head), 2);
			CU_ASSERT_FALSE(list_is_empty(&logop_node_1->head));

			list_for_each(pos2, logop_node_1->node_children_head)
			{
				if (j == 0) {
					logop_node_2 = list_entry(pos2, typeof(*logop_node_2), head);
					CU_ASSERT_EQUAL(logop_node_2->node_type, AST_TYPE_UPD_LOGOP);
					CU_ASSERT_EQUAL(logop_node_2->logop_type, AST_LOGOP_TYPE_OR);
					CU_ASSERT_EQUAL(list_length(logop_node_2->node_children_head), 2);
					CU_ASSERT_FALSE(list_is_empty(&logop_node_2->head));

					list_for_each(pos3, logop_node_2->node_children_head)
					{
						cmp_node_1 = list_entry(pos3, typeof(*cmp_node_1), head);
						CU_ASSERT_EQUAL(cmp_node_1->node_type, AST_TYPE_UPD_CMP);
						CU_ASSERT_EQUAL(cmp_node_1->cmp_type, AST_CMP_EQUALS_OP);
						CU_ASSERT_EQUAL(list_length(cmp_node_1->node_children_head), 2);
						CU_ASSERT_FALSE(list_is_empty(&cmp_node_1->head));

						list_for_each(pos5, cmp_node_1->node_children_head)
						{
							val_node_2 = list_entry(pos5, typeof(*val_node_2), head);

							CU_ASSERT_EQUAL(val_node_2->node_type, AST_TYPE_UPD_EXPRVAL);
							CU_ASSERT_EQUAL(list_length(val_node_2->node_children_head), 0);
							CU_ASSERT_FALSE(list_is_empty(&val_node_2->head));

							if (k == 0) {
								CU_ASSERT(val_node_2->value_type.is_name);
								CU_ASSERT_STRING_EQUAL(val_node_2->name_val, "id");
							} else if (k == 1) {
								CU_ASSERT(val_node_2->value_type.is_intnum);
								CU_ASSERT_EQUAL(val_node_2->int_val, 1);
							} else if (k == 2) {
								CU_ASSERT(val_node_2->value_type.is_name);
								CU_ASSERT_STRING_EQUAL(val_node_2->name_val, "id");
							} else if (k == 3) {
								CU_ASSERT(val_node_2->value_type.is_intnum);
								CU_ASSERT_EQUAL(val_node_2->int_val, 2);
							}

							k++;
						}
					}

				} else {
					cmp_node_2 = list_entry(pos2, typeof(*cmp_node_2), head);
					CU_ASSERT_EQUAL(cmp_node_2->node_type, AST_TYPE_UPD_CMP);
					CU_ASSERT_EQUAL(cmp_node_2->cmp_type, AST_CMP_EQUALS_OP);
					CU_ASSERT_EQUAL(list_length(cmp_node_2->node_children_head), 2);
					CU_ASSERT_FALSE(list_is_empty(&cmp_node_2->head));

					list_for_each(pos4, cmp_node_2->node_children_head)
					{
						val_node_3 = list_entry(pos4, typeof(*val_node_3), head);

						CU_ASSERT_EQUAL(val_node_3->node_type, AST_TYPE_UPD_EXPRVAL);
						CU_ASSERT_EQUAL(list_length(val_node_3->node_children_head), 0);
						CU_ASSERT_FALSE(list_is_empty(&val_node_3->head));

						if (l == 0) {
							CU_ASSERT(val_node_3->value_type.is_name);
							CU_ASSERT_STRING_EQUAL(val_node_3->name_val, "id");
						} else {
							CU_ASSERT(val_node_3->value_type.is_intnum);
							CU_ASSERT_EQUAL(val_node_3->int_val, 3);
						}

						l++;
					}
				}
				j++;
			}

		}
		i++;
	}

	queue_free(&ct);
	ast_free(root);
}

static void update_case_5(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_upd_update_node *update_node;
	struct ast_upd_assign_node *assign_node;
	struct ast_upd_isxin_node *isxin_node;
	struct ast_upd_exprval_node *val_node;
	struct list_head *pos1, *pos2;
	int i = 0, j = 0;

	parse_stmt("UPDATE A SET id = 42 WHERE id IN (1,2);", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	update_node = (typeof(update_node))root;
	CU_ASSERT_EQUAL(update_node->node_type, AST_TYPE_UPD_UPDATE);
	CU_ASSERT_EQUAL(list_length(update_node->node_children_head), 2);
	CU_ASSERT_STRING_EQUAL(update_node->table_name, "A");
	CU_ASSERT(list_is_empty(&update_node->head));

	list_for_each(pos1, update_node->node_children_head)
	{
		if (i == 0) {
			assign_node = list_entry(pos1, typeof(*assign_node), head);
			CU_ASSERT_EQUAL(assign_node->node_type, AST_TYPE_UPD_ASSIGN);
			CU_ASSERT_STRING_EQUAL(assign_node->field_name, "id");
			CU_ASSERT_FALSE(list_is_empty(&assign_node->head));
			CU_ASSERT_EQUAL(list_length(assign_node->node_children_head), 1);

			list_for_each(pos2, assign_node->node_children_head)
			{
				val_node = list_entry(pos2, typeof(*val_node), head);
				CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_UPD_EXPRVAL);
				CU_ASSERT(val_node->value_type.is_intnum);
				CU_ASSERT_EQUAL(val_node->int_val, 42);
				CU_ASSERT_FALSE(list_is_empty(&val_node->head));
				CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);

			}
		} else if (i == 1) {
			isxin_node = list_entry(pos1, typeof(*isxin_node), head);
			CU_ASSERT_EQUAL(isxin_node->node_type, AST_TYPE_UPD_EXPRISXIN);
			CU_ASSERT_FALSE(isxin_node->is_negation);
			CU_ASSERT_EQUAL(list_length(isxin_node->node_children_head), 3);
			CU_ASSERT_FALSE(list_is_empty(&isxin_node->head));

			list_for_each(pos2, isxin_node->node_children_head)
			{
				val_node = list_entry(pos2, typeof(*val_node), head);
				CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_UPD_EXPRVAL);
				CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);
				CU_ASSERT_FALSE(list_is_empty(&val_node->head));

				if (j == 0) {
					CU_ASSERT(val_node->value_type.is_name);
					CU_ASSERT_STRING_EQUAL(val_node->name_val, "id");
				} else if (j == 1) {
					CU_ASSERT(val_node->value_type.is_intnum);
					CU_ASSERT_EQUAL(val_node->int_val, 1);
				} else {
					CU_ASSERT(val_node->value_type.is_intnum);
					CU_ASSERT_EQUAL(val_node->int_val, 2);
				}
				j++;
			}

		}
		i++;
	}

	queue_free(&ct);
	ast_free(root);
}

static void update_case_6(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_upd_update_node *update_node;
	struct ast_upd_assign_node *assign_node;
	struct ast_upd_isxin_node *isxin_node;
	struct ast_upd_exprval_node *val_node;
	struct list_head *pos1, *pos2;
	int i = 0, j = 0;

	parse_stmt("UPDATE A SET id = 42 WHERE id NOT IN (1,2);", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	update_node = (typeof(update_node))root;
	CU_ASSERT_EQUAL(update_node->node_type, AST_TYPE_UPD_UPDATE);
	CU_ASSERT_EQUAL(list_length(update_node->node_children_head), 2);
	CU_ASSERT_STRING_EQUAL(update_node->table_name, "A");
	CU_ASSERT(list_is_empty(&update_node->head));

	list_for_each(pos1, update_node->node_children_head)
	{
		if (i == 0) {
			assign_node = list_entry(pos1, typeof(*assign_node), head);
			CU_ASSERT_EQUAL(assign_node->node_type, AST_TYPE_UPD_ASSIGN);
			CU_ASSERT_STRING_EQUAL(assign_node->field_name, "id");
			CU_ASSERT_FALSE(list_is_empty(&assign_node->head));
			CU_ASSERT_EQUAL(list_length(assign_node->node_children_head), 1);

			list_for_each(pos2, assign_node->node_children_head)
			{
				val_node = list_entry(pos2, typeof(*val_node), head);
				CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_UPD_EXPRVAL);
				CU_ASSERT(val_node->value_type.is_intnum);
				CU_ASSERT_EQUAL(val_node->int_val, 42);
				CU_ASSERT_FALSE(list_is_empty(&val_node->head));
				CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);

			}
		} else if (i == 1) {
			isxin_node = list_entry(pos1, typeof(*isxin_node), head);
			CU_ASSERT_EQUAL(isxin_node->node_type, AST_TYPE_UPD_EXPRISXIN);
			CU_ASSERT(isxin_node->is_negation);
			CU_ASSERT_EQUAL(list_length(isxin_node->node_children_head), 3);
			CU_ASSERT_FALSE(list_is_empty(&isxin_node->head));

			list_for_each(pos2, isxin_node->node_children_head)
			{
				val_node = list_entry(pos2, typeof(*val_node), head);
				CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_UPD_EXPRVAL);
				CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);
				CU_ASSERT_FALSE(list_is_empty(&val_node->head));

				if (j == 0) {
					CU_ASSERT(val_node->value_type.is_name);
					CU_ASSERT_STRING_EQUAL(val_node->name_val, "id");
				} else if (j == 1) {
					CU_ASSERT(val_node->value_type.is_intnum);
					CU_ASSERT_EQUAL(val_node->int_val, 1);
				} else {
					CU_ASSERT(val_node->value_type.is_intnum);
					CU_ASSERT_EQUAL(val_node->int_val, 2);
				}
				j++;
			}

		}
		i++;
	}

	queue_free(&ct);
	ast_free(root);
}

static void update_case_7(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_upd_update_node *update_node;
	struct ast_upd_assign_node *assign_node;
	struct ast_upd_isxnull_node *isxnull;
	struct ast_upd_exprval_node *val_node;
	struct list_head *pos1, *pos2;
	int i = 0;

	parse_stmt("UPDATE A SET id = 42 WHERE dob IS NULL;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	update_node = (typeof(update_node))root;
	CU_ASSERT_EQUAL(update_node->node_type, AST_TYPE_UPD_UPDATE);
	CU_ASSERT_EQUAL(list_length(update_node->node_children_head), 2);
	CU_ASSERT_STRING_EQUAL(update_node->table_name, "A");
	CU_ASSERT(list_is_empty(&update_node->head));

	list_for_each(pos1, update_node->node_children_head)
	{
		if (i == 0) {
			assign_node = list_entry(pos1, typeof(*assign_node), head);
			CU_ASSERT_EQUAL(assign_node->node_type, AST_TYPE_UPD_ASSIGN);
			CU_ASSERT_STRING_EQUAL(assign_node->field_name, "id");
			CU_ASSERT_FALSE(list_is_empty(&assign_node->head));
			CU_ASSERT_EQUAL(list_length(assign_node->node_children_head), 1);

			list_for_each(pos2, assign_node->node_children_head)
			{
				val_node = list_entry(pos2, typeof(*val_node), head);
				CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_UPD_EXPRVAL);
				CU_ASSERT(val_node->value_type.is_intnum);
				CU_ASSERT_EQUAL(val_node->int_val, 42);
				CU_ASSERT_FALSE(list_is_empty(&val_node->head));
				CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);

			}
		} else if (i == 1) {

			isxnull = list_entry(pos1, typeof(*isxnull), head);
			CU_ASSERT_EQUAL(isxnull->node_type, AST_TYPE_UPD_EXPRISXNULL);
			CU_ASSERT_FALSE(isxnull->is_negation);
			CU_ASSERT_EQUAL(list_length(isxnull->node_children_head), 1);
			CU_ASSERT_FALSE(list_is_empty(&isxnull->head));

			list_for_each(pos2, isxnull->node_children_head)
			{
				val_node = list_entry(pos2, typeof(*val_node), head);
				CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_UPD_EXPRVAL);
				CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);
				CU_ASSERT_FALSE(list_is_empty(&val_node->head));

				CU_ASSERT(val_node->value_type.is_name);
				CU_ASSERT_STRING_EQUAL(val_node->name_val, "dob");
			}

		}
		i++;
	}

	queue_free(&ct);
	ast_free(root);
}

static void update_case_8(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_upd_update_node *update_node;
	struct ast_upd_assign_node *assign_node;
	struct ast_upd_isxnull_node *isxnull;
	struct ast_upd_exprval_node *val_node;
	struct list_head *pos1, *pos2;
	int i = 0;

	parse_stmt("UPDATE A SET id = 42 WHERE dob IS NOT NULL;", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	update_node = (typeof(update_node))root;
	CU_ASSERT_EQUAL(update_node->node_type, AST_TYPE_UPD_UPDATE);
	CU_ASSERT_EQUAL(list_length(update_node->node_children_head), 2);
	CU_ASSERT_STRING_EQUAL(update_node->table_name, "A");
	CU_ASSERT(list_is_empty(&update_node->head));

	list_for_each(pos1, update_node->node_children_head)
	{
		if (i == 0) {
			assign_node = list_entry(pos1, typeof(*assign_node), head);
			CU_ASSERT_EQUAL(assign_node->node_type, AST_TYPE_UPD_ASSIGN);
			CU_ASSERT_STRING_EQUAL(assign_node->field_name, "id");
			CU_ASSERT_FALSE(list_is_empty(&assign_node->head));
			CU_ASSERT_EQUAL(list_length(assign_node->node_children_head), 1);

			list_for_each(pos2, assign_node->node_children_head)
			{
				val_node = list_entry(pos2, typeof(*val_node), head);
				CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_UPD_EXPRVAL);
				CU_ASSERT(val_node->value_type.is_intnum);
				CU_ASSERT_EQUAL(val_node->int_val, 42);
				CU_ASSERT_FALSE(list_is_empty(&val_node->head));
				CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);

			}
		} else if (i == 1) {

			isxnull = list_entry(pos1, typeof(*isxnull), head);
			CU_ASSERT_EQUAL(isxnull->node_type, AST_TYPE_UPD_EXPRISXNULL);
			CU_ASSERT(isxnull->is_negation);
			CU_ASSERT_EQUAL(list_length(isxnull->node_children_head), 1);
			CU_ASSERT_FALSE(list_is_empty(&isxnull->head));

			list_for_each(pos2, isxnull->node_children_head)
			{
				val_node = list_entry(pos2, typeof(*val_node), head);
				CU_ASSERT_EQUAL(val_node->node_type, AST_TYPE_UPD_EXPRVAL);
				CU_ASSERT_EQUAL(list_length(val_node->node_children_head), 0);
				CU_ASSERT_FALSE(list_is_empty(&val_node->head));

				CU_ASSERT(val_node->value_type.is_name);
				CU_ASSERT_STRING_EQUAL(val_node->name_val, "dob");
			}

		}
		i++;
	}

	queue_free(&ct);
	ast_free(root);
}

void test_ast_build_tree_update(void)
{
	/* no where-clause */
	update_case_1();

	/* where-clause; single condition */
	update_case_2();

	/* where-clause; single condition; inverted order (id=1 -> 1=id) */
	update_case_3();

	/* where-clause; multiple condition; */
	update_case_4();

	/* where-clause; in-condition; */
	update_case_5();

	/* where-clause; in-condition; */
	update_case_6();

	/* where-clause; is null */
	update_case_7();

	/* where-clause; is not null */
	update_case_8();

}
