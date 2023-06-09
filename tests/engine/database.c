/*
 * midoridb.c
 *
 *  Created on: 9/06/2023
 *      Author: paulo
 */

#include <engine/database.h>
#include <tests/engine.h>

void test_database_open(void)
{
	struct database db = {0};
	CU_ASSERT(database_open(&db));
	CU_ASSERT_PTR_NOT_NULL(&db);
	CU_ASSERT_PTR_NOT_NULL(db.tables);
	database_close(&db);
}

void test_database_close(void)
{
	struct database db = {0};
	CU_ASSERT(database_open(&db));
	database_close(&db);
	CU_ASSERT_PTR_NULL(db.tables);
}

void test_database_add_table(void)
{
	struct database db = {0};
	struct table *table_1;
	struct table *table_2;
	struct hashtable_value *entry;

	CU_ASSERT(database_open(&db));

	/* insert table 1 */
	table_1 = table_init("test_123");
	CU_ASSERT(database_add_table(&db, table_1));

	/* check for table 1 */
	entry = hashtable_get(db.tables, table_1->name, sizeof(table_1->name));
	CU_ASSERT_PTR_NOT_NULL(entry);

	/* insert table 2 */
	table_2 = table_init("test_456");
	CU_ASSERT(database_add_table(&db, table_2));
	/* check for table 1 && table 2*/
	entry = hashtable_get(db.tables, table_1->name, sizeof(table_1->name));
	CU_ASSERT_PTR_NOT_NULL(entry);
	entry = hashtable_get(db.tables, table_1->name, sizeof(table_1->name));
	CU_ASSERT_PTR_NOT_NULL(entry);

	database_close(&db);

}
