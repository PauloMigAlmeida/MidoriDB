/*
 * executor.c
 *
 *  Created on: 8/06/2023
 *      Author: paulo
 */

#include <tests/engine.h>

void test_executor_run(void)
{
	/* CREATE statements */
	test_executor_create();

	/* INSERT statements */
	test_executor_insert();

	/* DELETE statements */
	test_executor_delete();

	/* UPDATE statements */
	test_executor_update();

	/* SELECT statements */
	test_executor_select();
}
