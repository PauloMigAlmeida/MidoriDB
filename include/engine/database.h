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
 * Returns: true if successful, false otherwise
 */
bool database_open(struct database *db);

/**
 * database_close - close a database
 * @db: database to close 
 */
void database_close(struct database *db);

/**
 * database_add_table - add a table to a database
 * @db: database to add table to
 * @table: table to add
 * 
 * Returns: true if successful, false otherwise
 */
bool database_add_table(struct database *db, struct table *table);

#endif /* INCLUDE_ENGINE_DATABASE_H_ */
