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
	/* DELETE */
	AST_TYPE_DEL_DELETEONE,
	AST_TYPE_DEL_EXPRVAL,
	AST_TYPE_DEL_CMP,
	AST_TYPE_DEL_LOGOP,
	AST_TYPE_DEL_EXPRISXIN,
	AST_TYPE_DEL_EXPRISXNULL,
	/* UPDATE */
	AST_TYPE_UPD_UPDATE,
	AST_TYPE_UPD_ASSIGN,
	AST_TYPE_UPD_EXPRVAL,
	AST_TYPE_UPD_CMP,
	AST_TYPE_UPD_LOGOP,
	AST_TYPE_UPD_EXPRISXIN,
	AST_TYPE_UPD_EXPRISXNULL,
	/* SELECT */
	AST_TYPE_SEL_EXPRVAL,
	AST_TYPE_SEL_EXPROP,
	AST_TYPE_SEL_ALIAS,
	AST_TYPE_SEL_TABLE,
	AST_TYPE_SEL_FIELDNAME,
	AST_TYPE_SEL_CMP,
	AST_TYPE_SEL_LOGOP,
	AST_TYPE_SEL_EXPRISXNULL,
	AST_TYPE_SEL_EXPRISXIN,
	AST_TYPE_SEL_COUNT,
	AST_TYPE_SEL_LIKE,
	AST_TYPE_SEL_ONEXPR,
	AST_TYPE_SEL_JOIN,
	AST_TYPE_SEL_WHERE,
	AST_TYPE_SEL_GROUPBY,
	AST_TYPE_SEL_ORDERBY,
};

enum ast_comparison_type {
	// '='
	AST_CMP_EQUALS_OP = 4,
	// '>='
	AST_CMP_GTE_OP = 6,
	// '>'
	AST_CMP_GT_OP = 2,
	// '<='
	AST_CMP_LTE_OP = 5,
	// '<'
	AST_CMP_LT_OP = 1,
	// '<>' or '!='
	AST_CMP_DIFF_OP = 3,
};

/* Logical Operation type */
enum ast_logop_type {
	AST_LOGOP_TYPE_AND,
	AST_LOGOP_TYPE_OR,
	AST_LOGOP_TYPE_XOR,
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

	struct {
		bool is_intnum;
		bool is_str;
		bool is_approxnum;
		bool is_bool;
		bool is_null;
		bool is_negation;
	} value_type;
	/* raw values */
	union {
		int64_t int_val;
		char str_val[65535 + 1 /* NUL char */]; // MAX VARCHAR on MySQL too
		double double_val;
		bool bool_val;
	};
	/* synthetic value - hold intermediate values extracted from raw values in the SQL stmt */
	time_t date_val;
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

/* Delete Statements - start */

// used for: "IS NULL" and "IS NOT NULL"
struct ast_del_isxnull_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* NOT null? */
	bool is_negation;
};

// used for: "IS IN" and "IS NOT IN" WHERE-clauses
struct ast_del_isxin_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* NOT in? */
	bool is_negation;
};

struct ast_del_logop_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* logical operator type */
	enum ast_logop_type logop_type;
};

struct ast_del_cmp_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* comparison type */
	enum ast_comparison_type cmp_type;
};

struct ast_del_exprval_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* value type */

	struct {
		bool is_name;
		bool is_intnum;
		bool is_str;
		bool is_approxnum;
		bool is_bool;
		bool is_null;
	} value_type;
	/* raw values */
	union {
		char name_val[TABLE_MAX_COLUMN_NAME + 1 /*NUL char */];
		int64_t int_val;
		char str_val[65535 + 1 /* NUL char */]; // MAX VARCHAR on MySQL too
		double double_val;
		bool bool_val;
	};
	/* synthetic value - hold intermediate values extracted from raw values in the SQL stmt */
	time_t date_val;
};

struct ast_del_deleteone_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* table name */
	char table_name[255 + 1 /*NUL char */];
};

/* Delete Statements - end */

/* Update Statements - start */

// used for: "IS NULL" and "IS NOT NULL"
struct ast_upd_isxnull_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* NOT null? */
	bool is_negation;
};

// used for: "IS IN" and "IS NOT IN" WHERE-clauses
struct ast_upd_isxin_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* NOT in? */
	bool is_negation;
};

struct ast_upd_logop_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* logical operator type */
	enum ast_logop_type logop_type;
};

struct ast_upd_cmp_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* comparison type */
	enum ast_comparison_type cmp_type;
};

struct ast_upd_exprval_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* value type */

	struct {
		bool is_name;
		bool is_intnum;
		bool is_str;
		bool is_approxnum;
		bool is_bool;
		bool is_null;
	} value_type;
	/* raw values */
	union {
		char name_val[TABLE_MAX_COLUMN_NAME + 1 /*NUL char */];
		int64_t int_val;
		char str_val[65535 + 1 /* NUL char */]; // MAX VARCHAR on MySQL too
		double double_val;
		bool bool_val;
	};
	/* synthetic value - hold intermediate values extracted from raw values in the SQL stmt */
	time_t date_val;
};

struct ast_upd_assign_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* field name */
	char field_name[TABLE_MAX_COLUMN_NAME + 1 /*NUL char */];
};

struct ast_upd_update_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* table name */
	char table_name[255 + 1 /*NUL char */];
};

/* Update Statements - end */

/* Select Statements - start */

struct ast_sel_exprval_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* value type */

	struct {
		bool is_name;
		bool is_intnum;
		bool is_str;
		bool is_approxnum;
		bool is_bool;
		bool is_null;
		bool is_negation;
	} value_type;
	/* raw values */
	union {
		char name_val[TABLE_MAX_COLUMN_NAME + 1 /*NUL char */];
		int64_t int_val;
		char str_val[65535 + 1 /* NUL char */]; // MAX VARCHAR on MySQL too
		double double_val;
		bool bool_val;
	};
	/* synthetic value - hold intermediate values extracted from raw values in the SQL stmt */
	time_t date_val;
};

enum ast_sel_expr_op_type {
	AST_SEL_EXPR_OP_ADD,
	AST_SEL_EXPR_OP_SUB,
	AST_SEL_EXPR_OP_MUL,
	AST_SEL_EXPR_OP_DIV,
	AST_SEL_EXPR_OP_MOD,
};

/* math operators */
struct ast_sel_exprop_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* math operator */
	enum ast_sel_expr_op_type op_type;
};

struct ast_sel_alias_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* alias value*/
	char alias_value[TABLE_MAX_COLUMN_NAME + 1 /*NUL char */];
};

struct ast_sel_fieldname_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* table name */
	char table_name[TABLE_MAX_NAME + 1 /*NUL char */];
	/* column name */
	char col_name[TABLE_MAX_COLUMN_NAME + 1 /*NUL char */];
};

struct ast_sel_table_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* table name */
	char table_name[TABLE_MAX_NAME + 1 /*NUL char */];
};

struct ast_sel_cmp_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* comparison type */
	enum ast_comparison_type cmp_type;
};

struct ast_sel_logop_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* logical operator type */
	enum ast_logop_type logop_type;
};

// used for: "IS NULL" and "IS NOT NULL"
struct ast_sel_isxnull_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* NOT null? */
	bool is_negation;
};

// used for: "IS IN" and "IS NOT IN" WHERE-clauses
struct ast_sel_isxin_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* NOT in? */
	bool is_negation;
};

struct ast_sel_count_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* COUNT(*)? */
	bool all;
};

struct ast_sel_like_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* LIKE NOT ? */
	bool negate;
};

struct ast_sel_onexpr_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
};

enum ast_sel_join_type {
	AST_SEL_JOIN_INNER = 1,
	AST_SEL_JOIN_LEFT = 2,
	AST_SEL_JOIN_RIGHT = 4,
	AST_SEL_JOIN_LEFT_OUTER = 8,
	AST_SEL_JOIN_RIGHT_OUTER = 10,
};

struct ast_sel_join_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
	/* kind of join */
	enum ast_sel_join_type join_type;
};

struct ast_sel_where_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
};

struct ast_sel_groupby_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
};

struct ast_sel_orderby_node {
	/* type of node */
	enum ast_node_type node_type;
	/* children if applicable */
	struct list_head *node_children_head;
	/* doubly-linked list head */
	struct list_head head;
};

/* Select Statements - end */

struct ast_node* ast_build_tree(struct queue *out);
void ast_free(struct ast_node *node);
struct ast_node* ast_create_build_tree(struct queue *parser);
struct ast_node* ast_insert_build_tree(struct queue *parser);
struct ast_node* ast_delete_build_tree(struct queue *parser);
struct ast_node* ast_update_build_tree(struct queue *parser);
struct ast_node* ast_select_build_tree(struct queue *parser);

#endif /* INCLUDE_PARSER_AST_H_ */
