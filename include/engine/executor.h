/*
 * executor.h
 *
 *  Created on: 8/06/2023
 *      Author: paulo
 */

#ifndef INCLUDE_ENGINE_EXECUTOR_H_
#define INCLUDE_ENGINE_EXECUTOR_H_

#include <compiler/common.h>
#include <parser/ast.h>
#include <engine/database.h>
#include <engine/query.h>

/**
 * Run the execution plan for the given AST node.
 * @param db database reference
 * @param node The AST node to run.
 * @param err_out The buffer to write the error message to.
 * @param err_out_size The size of the error buffer.
 * 
 * @return: 0 if successful, < 0 otherwise. See <error.h> for details.
 */
int executor_run(struct database *db, struct ast_node *node, struct query_output *output);

/**
 * Run the execution plan for CREATE statements
 * @param db database reference
 * @param node The AST node to run.
 * @param err_out The buffer to write the error message to.
 * @param err_out_size The size of the error buffer.
 * 
 * @return: 0 if successful, < 0 otherwise. See <error.h> for details.
 */
int executor_run_create_stmt(struct database *db, struct ast_crt_create_node *create_node, struct query_output *output);

/**
 * Run the execution plan for INSERT (VALUES) statements
 * @param db database reference
 * @param node The AST node to run.
 * @param err_out The buffer to write the error message to.
 * @param err_out_size The size of the error buffer.
 * 
 * @return: 0 if successful, < 0 otherwise. See <error.h> for details.
 */
int executor_run_insertvals_stmt(struct database *db, struct ast_ins_insvals_node *ins_node, struct query_output *output);

/**
 * Run the execution plan for DELETE statements
 * @param db database reference
 * @param node The AST node to run.
 * @param err_out The buffer to write the error message to.
 * @param err_out_size The size of the error buffer.
 *
 * @return: 0 if successful, < 0 otherwise. See <error.h> for details.
 */
int executor_run_deleteone_stmt(struct database *db, struct ast_del_deleteone_node *delete_node, struct query_output *output);

/**
 * Run the execution plan for UPDATE statements
 * @param db database reference
 * @param node The AST node to run.
 * @param err_out The buffer to write the error message to.
 * @param err_out_size The size of the error buffer.
 *
 * @return: 0 if successful, < 0 otherwise. See <error.h> for details.
 */
int executor_run_update_stmt(struct database *db, struct ast_upd_update_node *update_node, struct query_output *output);

/**
 * Run the execution plan for SELECT statements
 * @param db database reference
 * @param node The AST node to run.
 * @param err_out The buffer to write the error message to.
 * @param err_out_size The size of the error buffer.
 *
 * @return: 0 if successful, < 0 otherwise. See <error.h> for details.
 */
int executor_run_select_stmt(struct database *db, struct ast_sel_select_node *select_node, struct query_output *output);

#endif /* INCLUDE_ENGINE_EXECUTOR_H_ */
