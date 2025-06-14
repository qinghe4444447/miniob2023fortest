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

#include "sql/operator/physical_operator.h"

/**
 * @brief 选择/投影物理算子
 * @ingroup PhysicalOperator
 */
class ProjectPhysicalOperator : public PhysicalOperator {
public:
  ProjectPhysicalOperator() {}

  virtual ~ProjectPhysicalOperator() = default;

  void add_projection(const Field &field);
  void add_projection(const Table *table, const FieldMeta *field);
  void add_projection(const char *alias);

  void add_expression(std::unique_ptr<Expression> &expression);

  PhysicalOperatorType type() const override { return PhysicalOperatorType::PROJECT; }

  RC open(Trx *trx) override;
  RC next(Tuple *env_tuple) override;
  RC close() override;

  int cell_num() const { return tuple_.cell_num(); }

  Tuple *current_tuple() override;

private:
  ProjectTuple tuple_;
};
