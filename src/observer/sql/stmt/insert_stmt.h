/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */



#pragma once

#include "common/rc.h"
#include "sql/stmt/stmt.h"
#include <vector>

class Table;
class Db;

/**
 * @brief 插入语句
 * @ingroup Statement
 */
class InsertStmt : public Stmt {
public:
  InsertStmt() = default;
  InsertStmt(Table *table, std::vector<std::vector<Value>> records);

  StmtType type() const override { return StmtType::INSERT; }

public:
  static RC create(Db *db, const InsertSqlNode &insert_sql, Stmt *&stmt);

public:
  Table *table() const { return table_; }
  const std::vector<std::vector<Value>> records() const { return records_; }
  int value_amount() const { return value_amount_; }
  int record_amount() const { return record_amount_; }

private:
  static RC check_record(Db *db, Table *table, std::vector<Value> &record, const std::vector<std::string> &names);

private:
  Table *table_ = nullptr;
  std::vector<std::vector<Value>> records_;
  int value_amount_ = 0;
  int record_amount_ = 0;
};
