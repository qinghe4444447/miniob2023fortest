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

#include "storage/field/field_meta.h"
#include "storage/table/table.h"

/**
 * @brief 字段
 * 
 */
class Field {
public:
  Field() = default;
  Field(const Table *table, const FieldMeta *field) : table_(table), field_(field) {}
  Field(const Field &) = default;

  const Table *table() const { return table_; }
  const FieldMeta *meta() const { return field_; }

  AttrType attr_type() const { return field_->type(); }

  const char *table_name() const { return table_->name(); }
  const char *field_name() const { return field_->name(); }

  void set_table(const Table *table) { this->table_ = table; }
  void set_field(const FieldMeta *field) { this->field_ = field; }

  void set_int(Record &record, int value);
  int get_int(const Record &record);

  const char *get_data(const Record &record);

  std::strong_ordering operator<=>(const Field &other) const;

private:
  const Table *table_ = nullptr;
  const FieldMeta *field_ = nullptr;
};
