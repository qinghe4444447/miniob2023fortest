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

#include <string>
#include <vector>

#include "sql/stmt/stmt.h"

/**
 * @brief 事务的Begin 语句，现在什么成员都没有
 * @ingroup Statement
 */
class TrxBeginStmt : public Stmt {
public:
  TrxBeginStmt() {}
  virtual ~TrxBeginStmt() = default;

  StmtType type() const override { return StmtType::BEGIN; }

  static RC create(Stmt *&stmt) {
    stmt = new TrxBeginStmt();
    return RC::SUCCESS;
  }
};