/*
 * ast.c
 *
 *  Created on: 15/05/2023
 *      Author: paulo
 */

#include <tests/parser.h>


void test_ast_build_tree(void)
{

	/* CREATE tests */
	test_ast_build_tree_create();

	/* INSERT tests */
	test_ast_build_tree_insert();

	/**
	 *
	 * Parser -> Syntax -> Semantic -> Optimiser -> Execution Plan (This works for Select, Insert, Update, Delete)
	 * Parser -> Syntax -> Semantic -> Execution Plan (this works for CREATE, DROP?)
	 *
	 * Things to fix: 30/05/2023
	 *
	 * - tweak ast_build_tree so it can parse multiple STMTs in the future.
	 */

}
