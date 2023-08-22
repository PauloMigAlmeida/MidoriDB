/*
 * optimiser.h
 *
 *  Created on: 22/08/2023
 *      Author: paulo
 */

#ifndef INCLUDE_ENGINE_OPTIMISER_H_
#define INCLUDE_ENGINE_OPTIMISER_H_

#include <compiler/common.h>
#include <engine/query.h>
#include <parser/ast.h>

/**
 * Optimises the AST tree
 * 
 * @param db the database reference
 * @param node the AST node to optimise
 * @param output the query output
 * 
 * @return 0 if success, otherwise error
 */
int optimiser_run(struct database *db, struct ast_node *node, struct query_output *output);

/**
 * Optimises the AST tree for a INSERT INTO ... VALUES ... statement
 * 
 * @param db the database reference
 * @param node the AST node to optimise
 * @param output the query output
 * 
 * @return 0 if success, otherwise error
 */
int optimiser_run_insertvals_stmt(struct database *db, struct ast_ins_insvals_node *node, struct query_output *output);

#endif /* INCLUDE_ENGINE_OPTIMISER_H_ */
