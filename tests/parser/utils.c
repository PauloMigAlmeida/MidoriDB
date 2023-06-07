/*
 * utils.c
 *
 *  Created on: 7/06/2023
 *      Author: paulo
 */

#include <tests/parser.h>
#include <parser/syntax.h>

void print_queue(struct queue *ct)
{
	for (size_t i = 0; i < queue_length(ct); i++) {
		printf("queue pos: %lu, content: %s\n", i, (char*)queue_peek_pos(ct, i));
	}
	printf("\n");
}

void parse_stmt(char *stmt, struct queue *out)
{
	CU_ASSERT_FATAL(queue_init(out));
	CU_ASSERT_EQUAL(syntax_parse(stmt, out), 0);
	printf("\n");
	print_queue(out);
}

struct ast_node* build_ast_for_query(char *stmt)
{
	struct queue queue = {0};
	struct ast_node *ret;

	parse_stmt(stmt, &queue);
	ret = ast_build_tree(&queue);
	queue_free(&queue);
	return ret;
}
