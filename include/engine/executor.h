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
int executor_run(struct database *db, struct ast_node *node, struct query_output* output);

#endif /* INCLUDE_ENGINE_EXECUTOR_H_ */
