/*
 * database.h
 *
 *  Created on: 8/06/2023
 *      Author: paulo
 */

#ifndef INCLUDE_ENGINE_DATABASE_H_
#define INCLUDE_ENGINE_DATABASE_H_

#include <compiler/common.h>
#include <primitive/table.h>
#include <datastructure/hashtable.h>

struct database {
    struct hashtable *tables;
    pthread_mutex_t mutex;
};

/**
 * database_open - initialise a database
 * @db: database to initialise
 * 
 * Returns: 0 if successful, < 0 otherwise. See <error.h> for details.
 */
int database_open(struct database *db);

/**
 * database_close - close a database
 * @db: database to close 
 */
void database_close(struct database *db);

/**
 * database_table_add - add a table to a database
 * @db: database reference
 * @table: table to add
 * 
 * Returns: 0 if successful, < 0 otherwise. See <error.h> for details.
 */
int database_table_add(struct database *db, struct table *table);

/**
 * database_table_exists - check if table exist in the database
 * @db: database reference
 * @table_name: name of the table. NUL-terminated
 *
 * Returns: true if table exists, false otherwise
 */
bool database_table_exists(struct database *db, char *table_name);


#endif /* INCLUDE_ENGINE_DATABASE_H_ */
