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

/**
 * Run the execution plan for the given AST node.
 * @param node The AST node to run.
 * @param err_out The buffer to write the error message to.
 * @param err_out_size The size of the error buffer.
 * 
 * @return True if the execution was successful, false otherwise.
 */
bool executor_run(struct ast_node *node, char* err_out, size_t err_out_size);

#endif /* INCLUDE_ENGINE_EXECUTOR_H_ */
