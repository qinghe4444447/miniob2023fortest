/* Copyright (c) 2023 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */


#pragma once
#include <memory>

#include "common/rc.h"
#include "sql/stmt/create_table_stmt.h"
#include "sql/stmt/join_stmt.h"

class Stmt;
class CalcStmt;
class SelectStmt;
class FilterStmt;
class InsertStmt;
class DeleteStmt;
class ExplainStmt;
class LogicalOperator;
class UpdateStmt;

class LogicalPlanGenerator {
public:
  LogicalPlanGenerator() = default;
  virtual ~LogicalPlanGenerator() = default;

  RC create(Stmt *stmt, std::unique_ptr<LogicalOperator> &logical_operator);

private:
  RC create_plan(CalcStmt *calc_stmt, std::unique_ptr<LogicalOperator> &logical_operator);
  RC create_plan(SelectStmt *select_stmt, std::unique_ptr<LogicalOperator> &logical_operator, bool readonly = true);
  RC create_plan(InsertStmt *insert_stmt, std::unique_ptr<LogicalOperator> &logical_operator);
  RC create_plan(DeleteStmt *delete_stmt, std::unique_ptr<LogicalOperator> &logical_operator);
  RC create_plan(ExplainStmt *explain_stmt, std::unique_ptr<LogicalOperator> &logical_operator);
  RC create_plan(JoinStmt *join_stmt, const set<Field> &fields, std::unique_ptr<LogicalOperator> &logical_operator,
                 bool readonly = true);
  RC create_plan(UpdateStmt *update_stmt, std::unique_ptr<LogicalOperator> &logical_operator);
  RC create_plan(CreateTableStmt *create_table_stmt, std::unique_ptr<LogicalOperator> &logical_operator);

  RC get_table_get_operator(Table *table, std::vector<Field> fields, bool readonly,
                            std::unique_ptr<LogicalOperator> &logical_operator);
};
