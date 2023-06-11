/*
 * executor.c
 *
 *  Created on: 8/06/2023
 *      Author: paulo
 */

#include <tests/engine.h>
#include <engine/executor.h>
#include <datastructure/queue.h>
#include <parser/syntax.h>

//TODO I have similar functions on the parser/utils.c -> I need to centralise those
static void parse_stmt(char *stmt, struct queue *out)
{
	CU_ASSERT_FATAL(queue_init(out));
	CU_ASSERT_EQUAL(syntax_parse(stmt, out), 0);
}

static struct ast_node* build_ast_for_query(char *stmt)
{
	struct queue queue = {0};
	struct ast_node *ret;

	parse_stmt(stmt, &queue);
	ret = ast_build_tree(&queue);
	queue_free(&queue);
	return ret;
}

void test_executor_run(void)
{
	struct database db = {0};
	struct ast_node *node;
	struct query_output output = {0};
	struct hashtable_value *value;
	struct table *table;

	CU_ASSERT_EQUAL(database_open(&db), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 0);

	node = build_ast_for_query("CREATE TABLE TEST (f1 INT, f2 INT);");
	CU_ASSERT_EQUAL(executor_run(&db, node, &output), MIDORIDB_OK);
	CU_ASSERT_EQUAL(db.tables->count, 1);

	value = hashtable_get(db.tables, "TEST", 5);
	CU_ASSERT_PTR_NOT_NULL(value);
	table = *(struct table**)value->content;

	CU_ASSERT_STRING_EQUAL(table->name, "TEST");
	CU_ASSERT_EQUAL(table->column_count, 2);
	CU_ASSERT_EQUAL(table->free_dtbkl_offset, 0);

	CU_ASSERT_STRING_EQUAL(table->columns[0].name, "f1");
	CU_ASSERT_EQUAL(table->columns[0].type, CT_INTEGER);
	CU_ASSERT_EQUAL(table->columns[0].precision, 8);
	CU_ASSERT_FALSE(table->columns[0].indexed);

	CU_ASSERT_STRING_EQUAL(table->columns[1].name, "f2");
	CU_ASSERT_EQUAL(table->columns[1].type, CT_INTEGER);
	CU_ASSERT_EQUAL(table->columns[1].precision, 8);
	CU_ASSERT_FALSE(table->columns[1].indexed);

	ast_free(node);
	database_close(&db);
}
