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
#include <datastructure/stack.h>

enum ast_node_type {
	AST_TYPE_STMT,
	AST_TYPE_CREATE,
	AST_TYPE_COLUMNDEF,
};

struct ast_node {
	enum ast_node_type node_type;
	struct list_head *node_children_head;
};

/* Create Statements - start */
struct ast_create_node {
	enum ast_node_type node_type;
	struct list_head *node_children_head;
	char table_name[TABLE_MAX_NAME + 1 /*NUL char */]; //TODO add tests to check array bounds during parse-to-AST
	bool if_not_exists;
	int column_count;
};

struct ast_column_def_node {
	enum ast_node_type node_type;
	struct list_head *node_children_head;
	struct list_head head; /* doubly-linked list head */
	char name[TABLE_MAX_COLUMN_NAME + 1 /*NUL char */]; //TODO check boundaries
	enum COLUMN_TYPE type;
	size_t precision;
	bool attr_null;
	bool attr_not_null;
	bool attr_uniq_key;
	bool attr_auto_inc;
	bool attr_prim_key;
};
/* Create Statements - end */

struct ast_node* ast_build_tree(struct stack *out);

#endif /* INCLUDE_PARSER_AST_H_ */
