/*
 * utils.h
 *
 *  Created on: 29/06/2023
 *      Author: paulo
 */

#ifndef INCLUDE_TESTS_UTILS_H_
#define INCLUDE_TESTS_UTILS_H_

#include <compiler/common.h>
#include <primitive/table.h>
#include <primitive/datablock.h>
#include <primitive/row.h>

size_t count_datablocks(struct table *table);
struct row* fetch_row(struct table *table, size_t row_num);
bool check_row_data(struct table *table, size_t row_num, void *expected);
bool check_row_flags(struct table *table, size_t row_num, struct row_header_flags *expected);
bool check_row(struct table *table, size_t row_num, struct row_header_flags *exp_header_flags, struct row *row);
struct datablock* fetch_datablock(struct table *table, size_t idx);
struct row* build_row(void *data, size_t data_len, int *null_cols_idx, size_t cols_idx_len);

#endif /* INCLUDE_TESTS_UTILS_H_ */
