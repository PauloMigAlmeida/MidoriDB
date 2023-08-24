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

static void insert_table_case_1(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_ins_insvals_node *insert_node;
	struct list_head *pos1, *pos2;
	struct ast_ins_values_node *vals_entry;
	struct ast_ins_exprval_node *expval_entry;
	int i = 0;

	parse_stmt("INSERT INTO A VALUES (123, '456', true, 1.0);", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	insert_node = (typeof(insert_node))root;
	CU_ASSERT_EQUAL(insert_node->node_type, AST_TYPE_INS_INSVALS);
	CU_ASSERT_EQUAL(insert_node->row_count, 1);
	CU_ASSERT_EQUAL(list_length(insert_node->node_children_head), 1);
	CU_ASSERT_STRING_EQUAL(insert_node->table_name, "A");
	CU_ASSERT(list_is_empty(&insert_node->head));
	CU_ASSERT_FALSE(insert_node->opt_column_list);

	list_for_each(pos1, insert_node->node_children_head)
	{
		vals_entry = list_entry(pos1, typeof(*vals_entry), head);
		CU_ASSERT_EQUAL(vals_entry->node_type, AST_TYPE_INS_VALUES);
		CU_ASSERT_FALSE(list_is_empty(&vals_entry->head));
		CU_ASSERT_EQUAL(list_length(vals_entry->node_children_head), 4);

		list_for_each(pos2, vals_entry->node_children_head)
		{
			expval_entry = list_entry(pos2, typeof(*expval_entry), head);

			CU_ASSERT_EQUAL(expval_entry->node_type, AST_TYPE_INS_EXPRVAL);
			CU_ASSERT_FALSE(list_is_empty(&expval_entry->head));
			CU_ASSERT_EQUAL(list_length(expval_entry->node_children_head), 0);

			if (i == 0) {
				CU_ASSERT(expval_entry->value_type.is_intnum);
				CU_ASSERT_EQUAL(expval_entry->int_val, 123);
			} else if (i == 1) {
				CU_ASSERT(expval_entry->value_type.is_str);
				CU_ASSERT_STRING_EQUAL(expval_entry->str_val, "456");
			} else if (i == 2) {
				CU_ASSERT(expval_entry->value_type.is_bool);
				CU_ASSERT(expval_entry->bool_val);
			} else {
				CU_ASSERT(expval_entry->value_type.is_approxnum);
				CU_ASSERT_EQUAL(expval_entry->double_val, 1.0);
			}
			i++;
		}

	}

	queue_free(&ct);
	ast_free(root);
}

static void insert_table_case_2(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_ins_insvals_node *insert_node;
	struct list_head *pos1, *pos2;
	struct ast_ins_values_node *vals_entry;
	struct ast_ins_exprval_node *expval_entry;
	struct ast_ins_inscols_node *inscols_node;
	struct ast_ins_column_node *col_entry;
	struct ast_node *tmp_entry;
	int i = 0, j = 0;

	parse_stmt("INSERT INTO A (f1, f2) VALUES (123, '456'),(789, '012');", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	insert_node = (typeof(insert_node))root;
	CU_ASSERT_EQUAL(insert_node->node_type, AST_TYPE_INS_INSVALS);
	CU_ASSERT_EQUAL(insert_node->row_count, 2);
	/* Row 1 + Row 2 + Ins Col (yep, they count too) */
	CU_ASSERT_EQUAL(list_length(insert_node->node_children_head), 3);
	CU_ASSERT_STRING_EQUAL(insert_node->table_name, "A");
	CU_ASSERT(insert_node->opt_column_list);
	CU_ASSERT(list_is_empty(&insert_node->head));

	list_for_each(pos1, insert_node->node_children_head)
	{
		tmp_entry = list_entry(pos1, typeof(*tmp_entry), head);
		j = 0;

		if (i == 0) {
			inscols_node = (struct ast_ins_inscols_node*)tmp_entry;
			CU_ASSERT_EQUAL(inscols_node->node_type, AST_TYPE_INS_INSCOLS);
			CU_ASSERT_FALSE(list_is_empty(&inscols_node->head));
			CU_ASSERT_EQUAL(list_length(inscols_node->node_children_head), 2);

			list_for_each(pos2, inscols_node->node_children_head)
			{
				col_entry = list_entry(pos2, typeof(*col_entry), head);

				CU_ASSERT_EQUAL(col_entry->node_type, AST_TYPE_INS_COLUMN);
				CU_ASSERT_FALSE(list_is_empty(&col_entry->head));
				CU_ASSERT_EQUAL(list_length(col_entry->node_children_head), 0);

				if (j == 0) {
					CU_ASSERT_STRING_EQUAL(col_entry->name, "f1");
				} else {
					CU_ASSERT_STRING_EQUAL(col_entry->name, "f2");
				}
				j++;
			}
		} else {
			vals_entry = (struct ast_ins_values_node*)tmp_entry;
			CU_ASSERT_EQUAL(vals_entry->node_type, AST_TYPE_INS_VALUES);
			CU_ASSERT_FALSE(list_is_empty(&vals_entry->head));
			CU_ASSERT_EQUAL(list_length(vals_entry->node_children_head), 2);

			list_for_each(pos2, vals_entry->node_children_head)
			{
				expval_entry = list_entry(pos2, typeof(*expval_entry), head);

				CU_ASSERT_EQUAL(expval_entry->node_type, AST_TYPE_INS_EXPRVAL);
				CU_ASSERT_FALSE(list_is_empty(&expval_entry->head));
				CU_ASSERT_EQUAL(list_length(expval_entry->node_children_head), 0);

				if (j == 0) {
					CU_ASSERT(expval_entry->value_type.is_intnum);

					if (i == 1) {
						CU_ASSERT_EQUAL(expval_entry->int_val, 123);
					} else {
						CU_ASSERT_EQUAL(expval_entry->int_val, 789);
					}
				} else {
					CU_ASSERT(expval_entry->value_type.is_str);

					if (i == 1) {
						CU_ASSERT_STRING_EQUAL(expval_entry->str_val, "456");
					} else {
						CU_ASSERT_STRING_EQUAL(expval_entry->str_val, "012");
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

static void insert_table_case_3(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_ins_insvals_node *insert_node;
	struct list_head *pos1, *pos2, *pos3, *pos4;
	struct ast_ins_values_node *vals_entry;
	struct ast_ins_exprval_node *expval_entry1, *expval_entry2;
	struct ast_ins_exprop_node *expop_entry1, *expop_entry2;
	int i = 0;
	int j = 0;

	parse_stmt("INSERT INTO A VALUES ((2 + 3) * 3);", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	insert_node = (typeof(insert_node))root;
	CU_ASSERT_EQUAL(insert_node->node_type, AST_TYPE_INS_INSVALS);
	CU_ASSERT_EQUAL(insert_node->row_count, 1);
	CU_ASSERT_EQUAL(list_length(insert_node->node_children_head), 1);
	CU_ASSERT_STRING_EQUAL(insert_node->table_name, "A");
	CU_ASSERT(list_is_empty(&insert_node->head));
	CU_ASSERT_FALSE(insert_node->opt_column_list);

	list_for_each(pos1, insert_node->node_children_head)
	{
		vals_entry = list_entry(pos1, typeof(*vals_entry), head);

		CU_ASSERT_EQUAL(vals_entry->node_type, AST_TYPE_INS_VALUES);
		CU_ASSERT_FALSE(list_is_empty(&vals_entry->head));
		CU_ASSERT_EQUAL(list_length(vals_entry->node_children_head), 1);

		list_for_each(pos2, vals_entry->node_children_head)
		{
			expop_entry1 = list_entry(pos2, typeof(*expop_entry1), head);

			CU_ASSERT_EQUAL(expop_entry1->node_type, AST_TYPE_INS_EXPROP);
			CU_ASSERT_EQUAL(expop_entry1->op_type, AST_INS_EXPR_OP_MUL);
			CU_ASSERT_FALSE(list_is_empty(&expop_entry1->head));
			CU_ASSERT_EQUAL(list_length(expop_entry1->node_children_head), 2);

			list_for_each(pos3, expop_entry1->node_children_head)
			{
				if (i == 0) {
					expval_entry1 = list_entry(pos3, typeof(*expval_entry1), head);
					CU_ASSERT(expval_entry1->value_type.is_intnum);
					CU_ASSERT_EQUAL(expval_entry1->int_val, 3);
					CU_ASSERT_FALSE(list_is_empty(&expval_entry1->head));
					CU_ASSERT_EQUAL(list_length(expval_entry1->node_children_head), 0);
				} else {
					expop_entry2 = list_entry(pos3, typeof(*expop_entry2), head);
					CU_ASSERT_EQUAL(expop_entry2->node_type, AST_TYPE_INS_EXPROP);
					CU_ASSERT_EQUAL(expop_entry2->op_type, AST_INS_EXPR_OP_ADD);
					CU_ASSERT_FALSE(list_is_empty(&expop_entry2->head));
					CU_ASSERT_EQUAL(list_length(expop_entry2->node_children_head), 2);

					list_for_each(pos4, expop_entry2->node_children_head)
					{
						expval_entry2 = list_entry(pos4, typeof(*expval_entry2), head);
						CU_ASSERT(expval_entry2->value_type.is_intnum);
						CU_ASSERT_FALSE(list_is_empty(&expval_entry2->head));
						CU_ASSERT_EQUAL(list_length(expval_entry2->node_children_head), 0);

						if (j == 0) {
							CU_ASSERT_EQUAL(expval_entry2->int_val, 3);
						} else {
							CU_ASSERT_EQUAL(expval_entry2->int_val, 2);
						}
						j++;
					}
				}
				i++;
			}
		}

	}

	queue_free(&ct);
	ast_free(root);
}

static void insert_table_case_4(void)
{
	struct queue ct = {0};
	struct ast_node *root;
	struct ast_ins_insvals_node *insert_node;
	struct list_head *pos1, *pos2, *pos3, *pos4, *pos5;
	struct ast_ins_values_node *vals_entry;
	struct ast_ins_exprval_node *expval_entry1, *expval_entry2, *expval_entry3;
	struct ast_ins_exprop_node *expop_entry1, *expop_entry2, *expop_entry3;
	int i = 0, j = 0, k = 0;

	parse_stmt("INSERT INTO A VALUES (-(2 + 3) * 2);", &ct);

	root = ast_build_tree(&ct);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);

	insert_node = (typeof(insert_node))root;
	CU_ASSERT_EQUAL(insert_node->node_type, AST_TYPE_INS_INSVALS);
	CU_ASSERT_EQUAL(insert_node->row_count, 1);
	CU_ASSERT_EQUAL(list_length(insert_node->node_children_head), 1);
	CU_ASSERT_STRING_EQUAL(insert_node->table_name, "A");
	CU_ASSERT(list_is_empty(&insert_node->head));
	CU_ASSERT_FALSE(insert_node->opt_column_list);

	list_for_each(pos1, insert_node->node_children_head)
	{
		vals_entry = list_entry(pos1, typeof(*vals_entry), head);

		CU_ASSERT_EQUAL(vals_entry->node_type, AST_TYPE_INS_VALUES);
		CU_ASSERT_FALSE(list_is_empty(&vals_entry->head));
		CU_ASSERT_EQUAL(list_length(vals_entry->node_children_head), 1);

		list_for_each(pos2, vals_entry->node_children_head)
		{
			expop_entry1 = list_entry(pos2, typeof(*expop_entry1), head);

			CU_ASSERT_EQUAL(expop_entry1->node_type, AST_TYPE_INS_EXPROP);
			CU_ASSERT_EQUAL(expop_entry1->op_type, AST_INS_EXPR_OP_MUL);
			CU_ASSERT_FALSE(list_is_empty(&expop_entry1->head));
			CU_ASSERT_EQUAL(list_length(expop_entry1->node_children_head), 2);

			list_for_each(pos3, expop_entry1->node_children_head)
			{
				if (i == 0) {
					expval_entry1 = list_entry(pos3, typeof(*expval_entry1), head);
					CU_ASSERT(expval_entry1->value_type.is_intnum);
					CU_ASSERT_EQUAL(expval_entry1->int_val, 2);
					CU_ASSERT_FALSE(list_is_empty(&expval_entry1->head));
					CU_ASSERT_EQUAL(list_length(expval_entry1->node_children_head), 0);
				} else {
					expop_entry2 = list_entry(pos3, typeof(*expop_entry2), head);
					CU_ASSERT_EQUAL(expop_entry2->node_type, AST_TYPE_INS_EXPROP);
					CU_ASSERT_EQUAL(expop_entry2->op_type, AST_INS_EXPR_OP_MUL); // Negation
					CU_ASSERT_FALSE(list_is_empty(&expop_entry2->head));
					CU_ASSERT_EQUAL(list_length(expop_entry2->node_children_head), 2);

					list_for_each(pos4, expop_entry2->node_children_head)
					{
						if (j == 0) {
							expval_entry2 = list_entry(pos4, typeof(*expval_entry2), head);
							CU_ASSERT_EQUAL(expval_entry2->node_type, AST_TYPE_INS_EXPRVAL);
							CU_ASSERT(expval_entry2->value_type.is_negation);
							CU_ASSERT_EQUAL(expval_entry2->int_val, -1);
							CU_ASSERT_FALSE(list_is_empty(&expval_entry2->head));
							CU_ASSERT_EQUAL(list_length(expval_entry2->node_children_head),
									0);
						} else {
							expop_entry3 = list_entry(pos4, typeof(*expop_entry3), head);
							CU_ASSERT_EQUAL(expop_entry3->node_type, AST_TYPE_INS_EXPROP);
							CU_ASSERT_EQUAL(expop_entry3->op_type, AST_INS_EXPR_OP_ADD);
							CU_ASSERT_FALSE(list_is_empty(&expop_entry3->head));
							CU_ASSERT_EQUAL(list_length(expop_entry3->node_children_head),
									2);

							list_for_each(pos5, expop_entry3->node_children_head)
							{
								// the 2 entries have the same value -> 2 , 2
								expval_entry3 = list_entry(pos5, typeof(*expval_entry3),
												head);
								CU_ASSERT(expval_entry3->value_type.is_intnum);

								CU_ASSERT_FALSE(list_is_empty(&expval_entry3->head));
								CU_ASSERT_EQUAL(list_length(
										expval_entry3->node_children_head),
										0);
								if (k == 0) {
									CU_ASSERT_EQUAL(expval_entry3->int_val, 3);
								} else {
									CU_ASSERT_EQUAL(expval_entry3->int_val, 2);
								}
								k++;

							}
						}
						j++;

					}
				}
				i++;
			}
		}

	}

	queue_free(&ct);
	ast_free(root);
}

void test_ast_build_tree_insert(void)
{
	/* insert - no col_names; single row */
	insert_table_case_1();
	/* insert - with col_names; multiple rows */
	insert_table_case_2();
	/* insert - no col_names; single row; recursive math expr */
	insert_table_case_3();
	/* insert - no col_names; single row; recursive math expr; negation operator (which add ghost nodes) */
	insert_table_case_4();

	/* TODO: insert - no col_names; select */
	/* TODO: insert - with col_names; select */
}
