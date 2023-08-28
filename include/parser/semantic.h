/*
 * semantic.h
 *
 *  Created on: 7/06/2023
 *      Author: paulo
 */

#ifndef INCLUDE_PARSER_SEMANTIC_H_
#define INCLUDE_PARSER_SEMANTIC_H_

#include <compiler/common.h>
#include <engine/database.h>
#include <parser/ast.h>

/*
 * semantic_analyse - Performs semantic analysis on the AST
 * @db database reference
 * @node: AST node to be analyzed
 * @out_err: buffer to store the error message
 * @out_err_len: length of the buffer
 *
 * this function returns true if the semantic analysis was successful, false otherwise.
 */
bool semantic_analyse(struct database *db, struct ast_node *node, char *out_err, size_t out_err_len);

#endif /* INCLUDE_PARSER_SEMANTIC_H_ */
