/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */



#include "sql/operator/project_physical_operator.h"
#include "common/log/log.h"
#include "sql/expr/tuple_cell.h"
#include "sql/operator/string_list_physical_operator.h"
#include "storage/record/record.h"
#include "storage/table/table.h"
#include <memory>
#include <utility>

RC ProjectPhysicalOperator::open(Trx *trx) {
  if (children_.empty()) {
    auto string = make_unique<StringListPhysicalOperator>();
    string->append("");
    add_child(std::move(string));
  }

  PhysicalOperator *child = children_[0].get();
  RC rc = child->open(trx);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  return RC::SUCCESS;
}

RC ProjectPhysicalOperator::next(Tuple *env_tuple) {
  if (children_.empty()) {
    return RC::RECORD_EOF;
  }
  return children_[0]->next(env_tuple);
}

RC ProjectPhysicalOperator::close() {
  if (!children_.empty()) {
    children_[0]->close();
  }
  return RC::SUCCESS;
}
Tuple *ProjectPhysicalOperator::current_tuple() {
  tuple_.set_tuple(children_[0]->current_tuple());
  return &tuple_;
}

void ProjectPhysicalOperator::add_projection(const Table *table, const FieldMeta *field_meta) {
  // 对单表来说，展示的(alias) 字段总是字段名称，
  // 对多表查询来说，展示的alias 需要带表名字
  TupleCellSpec *spec = new TupleCellSpec(table->name(), field_meta->name());
  tuple_.add_cell_spec(spec);
}

void ProjectPhysicalOperator::add_projection(const char *alias) {
  TupleCellSpec *spec = new TupleCellSpec(alias);
  tuple_.add_cell_spec(spec);
}

void ProjectPhysicalOperator::add_projection(const Field &field) { add_projection(field.table(), field.meta()); }

void ProjectPhysicalOperator::add_expression(std::unique_ptr<Expression> &expression) {
  tuple_.add_expression(expression);
}
