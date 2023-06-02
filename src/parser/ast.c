/*
 * ast.c
 *
 *  Created on: 15/05/2023
 *      Author: paulo
 */

#include <parser/ast.h>
#include <lib/string.h>

extern struct ast_node* ast_create_build_tree(struct queue *parser);

struct ast_node* ast_build_tree(struct queue *parser)
{
	char *str = NULL;
	size_t pos = 0;
	bool found = false;

	/* sanity checks */
	BUG_ON(!parser || queue_empty(parser));
//	BUG_ON(!strstarts((char* stack_peek(parser), "STMT")); //TODO adapt this for queue

	/* walk to STMT */
	while (pos < queue_length(parser)) {
		if (strstarts((char*)queue_peek_pos(parser, pos), "STMT")) {
			found = true;
			break;
		}
		pos++;
	}

	/* something terribly wrong if this is true */
	BUG_ON(!found);

	/*
	 * Notes to my future self:
	 *
	 * right now I'm just parsing a single statement but should I need to parse multiple
	 * statements then this logic has to change
	 */

	/* decide which routine to best parse its contents based on the operation type */
	pos--;
	str = (char*)queue_peek_pos(parser, pos);
	if (strstarts(str, "CREATE")) {
		return ast_create_build_tree(parser);
	} else {
		fprintf(stderr, "%s: %s handler not implement yet\n", __func__, str);
		exit(1);
	}
}

void ast_free(struct ast_node *node)
{
	struct list_head *pos = NULL;
	struct list_head *tmp_pos = NULL;
	struct ast_node *entry = NULL;

	if (node->node_children_head) {

		list_for_each_safe(pos,tmp_pos, node->node_children_head)
		{
			entry = list_entry(pos, typeof(*entry), head);
			ast_free(entry);
		}

		free(node->node_children_head);
	}

	free(node);
}
