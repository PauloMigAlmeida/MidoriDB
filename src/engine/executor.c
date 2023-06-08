/*
 * executor.c
 *
 *  Created on: 8/06/2023
 *      Author: paulo
 */

#include <engine/executor.h>

bool executor_run(struct ast_node *node, char* err_out, size_t err_out_size)
{
    /* sanity checks */
    BUG_ON(!node || !err_out || err_out_size == 0);

    
    return false;
}
