/*
 * ast.h
 *
 *  Created on: 15/05/2023
 *      Author: paulo
 */

#ifndef INCLUDE_PARSER_AST_H_
#define INCLUDE_PARSER_AST_H_

#include <compiler/common.h>
#include <primitive/table.h>
#include <primitive/column.h>
#include <datastructure/linkedlist.h>
#include <datastructure/queue.h>

// AST_TYPE_<stmt-type>_<rpn-prefix>
enum ast_node_type {
	AST_TYPE_STMT,
	/* CREATE */
	AST_TYPE_CRT_CREATE,
	AST_TYPE_CRT_COLUMNDEF,
	AST_TYPE_CRT_INDEXDEF,
	AST_TYPE_CRT_INDEXCOL,
	/* INSERT */
	AST_TYPE_INS_COLUMN,
	AST_TYPE_INS_INSCOLS,
	AST_TYPE_INS_EXPRVAL,
	AST_TYPE_INS_EXPROP,
	AST_TYPE_INS_VALUES,
	AST_TYPE_INS_INSVALS,
};

struct ast_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
};

/* Create Statements - start */
struct ast_crt_create_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;

	char table_name[255 + 1 /*NUL char */];
	bool if_not_exists;
	int column_count;
};

struct ast_crt_column_def_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;

	char name[255 + 1 /*NUL char */];
	enum COLUMN_TYPE type;
	size_t precision;
	bool attr_null;
	bool attr_not_null;
	bool attr_uniq_key;
	bool attr_auto_inc;
	bool attr_prim_key;
};

/* Indexes -> Index || Primary Keys */
struct ast_crt_index_def_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;

	bool is_pk;
	bool is_index;
};

/* Columns referenced when creating index as the last few lines of a CREATE statement */
struct ast_crt_index_column_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;

	char name[255 + 1 /*NUL char */];
};
/* Create Statements - end */

/* Insert Statements - start */
enum ast_ins_expr_op_type {
	AST_INS_EXPR_OP_ADD,
	AST_INS_EXPR_OP_SUB,
	AST_INS_EXPR_OP_MUL,
	AST_INS_EXPR_OP_DIV,
	AST_INS_EXPR_OP_MOD,
};

/* columns referenced in the INSERT statement */
struct ast_ins_column_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* column name */
	char name[255 + 1 /*NUL char*/];
};

struct ast_ins_inscols_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* number of columns */
	int column_count;
};

/* raw values */
struct ast_ins_exprval_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* value type */
	bool is_intnum;
	bool is_str;
	bool is_approxnum;
	bool is_bool;
	bool is_null;
	/* actual value */
	union {
		int64_t int_val;
		char str_val[65535 + 1 /* NUL char */]; // MAX VARCHAR on MySQL too
		double double_val;
		bool bool_val;
	};
};

/* math operators */
struct ast_ins_exprop_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* math operator */
	enum ast_ins_expr_op_type op_type;
};

/* indicate how many values are being specified after the "VALUES" keyword */
struct ast_ins_values_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* number of values */
	int value_count;
};

/* how many rows will be inserted as part of a INSERT statement */
struct ast_ins_insvals_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* table name */
	char table_name[255 + 1 /*NUL char */];
	/* number of rows */
	int row_count;
	/* were columns specified as part of the INSERT stmt ? if so, there will be added to node_children_head too */
	bool opt_column_list;
};

/* Insert Statements - end */

struct ast_node* ast_build_tree(struct queue *out);
void ast_free(struct ast_node *node);

#endif /* INCLUDE_PARSER_AST_H_ */
