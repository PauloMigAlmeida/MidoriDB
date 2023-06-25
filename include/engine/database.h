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
 * Note: this method is not thread-safe. It is the caller's responsibility to
 * call database_lock() before calling this method.
 * 
 * Returns: 0 if successful, < 0 otherwise. See <error.h> for details.
 */
int database_table_add(struct database *db, struct table *table);

/**
 * database_table_get - get a table from a database
 * @db: database reference
 * @table_name: name of the table. NUL-terminated
 * 
 * Note: this method is not thread-safe. It is the caller's responsibility to
 * call database_lock() before calling this method.
 * 
 * Returns: table if successful, NULL otherwise
 */
struct table *database_table_get(struct database *db, char *table_name);

/**
 * database_table_exists - check if table exist in the database
 * @db: database reference
 * @table_name: name of the table. NUL-terminated
 *
 * Note: this method is not thread-safe. It is the caller's responsibility to
 * call database_lock() before calling this method.
 * 
 * Returns: true if table exists, false otherwise
 */
bool database_table_exists(struct database *db, char *table_name);

/**
 * database_lock - lock a database
 * @db: database to lock
 * 
 * Returns: 0 if successful, < 0 otherwise. See <error.h> for details.
 */
int __must_check database_lock(struct database *db);

/**
 * database_unlock - unlock a database
 * @db: database to unlock
 * 
 * Returns: 0 if successful, < 0 otherwise. See <error.h> for details.
 */
int database_unlock(struct database *db);

#endif /* INCLUDE_ENGINE_DATABASE_H_ */
