/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */



#include "storage/index/bplus_tree_index.h"
#include "common/log/log.h"
#include "storage/field/field.h"
#include <cstdlib>

BplusTreeIndex::~BplusTreeIndex() noexcept { close(); }

RC BplusTreeIndex::create(const char *file_name, Table *table, const IndexMeta &index_meta) {
  if (inited_) {
    LOG_WARN("Failed to create index due to the index has been created before. file_name:%s, index:%s, field:%s",
             file_name, index_meta.name(), index_meta.fields_name().c_str());
    return RC::RECORD_OPENNED;
  }

  Index::init(index_meta);

  RC rc = index_handler_.create(file_name, table, index_meta, size_);
  if (RC::SUCCESS != rc) {
    LOG_WARN("Failed to create index_handler, file_name:%s, index:%s, field:%s, rc:%s", file_name, index_meta.name(),
             index_meta.fields_name().c_str(), strrc(rc));
    return rc;
  }

  inited_ = true;
  LOG_INFO("Successfully create index, file_name:%s, index:%s, field:%s", file_name, index_meta.name(),
           index_meta.fields_name().c_str());
  return RC::SUCCESS;
}

RC BplusTreeIndex::open(const char *file_name, Table *table, const IndexMeta &index_meta) {
  if (inited_) {
    LOG_WARN("Failed to open index due to the index has been initedd before. file_name:%s, index:%s, field:%s",
             file_name, index_meta.name(), index_meta.fields_name().c_str());
    return RC::RECORD_OPENNED;
  }

  Index::init(index_meta);

  RC rc = index_handler_.open(file_name, table, index_meta);
  if (RC::SUCCESS != rc) {
    LOG_WARN("Failed to open index_handler, file_name:%s, index:%s, field:%s, rc:%s", file_name, index_meta.name(),
             index_meta.fields_name().c_str(), strrc(rc));
    return rc;
  }

  inited_ = true;
  LOG_INFO("Successfully open index, file_name:%s, index:%s, field:%s", file_name, index_meta.name(),
           index_meta.fields_name().c_str());
  return RC::SUCCESS;
}

RC BplusTreeIndex::close() {
  if (inited_) {
    LOG_INFO("Begin to close index, index:%s, field:%s", index_meta_.name(), index_meta_.fields_name().c_str());
    index_handler_.close();
    inited_ = false;
  }
  LOG_INFO("Successfully close index.");
  return RC::SUCCESS;
}

char *BplusTreeIndex::make_key(const char *record) {
  char *data = (char *)malloc(size_);
  int beg = 0;
  for (auto &field : index_meta_.fields()) {
    memcpy(data + beg, record + field.offset(), field.len());
    beg += field.len();
  }
  return data;
}

RC BplusTreeIndex::insert_entry(const char *record, const RID *rid) {
  char *data = make_key(record);
  RC rc = index_handler_.insert_entry(data, rid);
  free(data);
  return rc;
}

RC BplusTreeIndex::delete_entry(const char *record, const RID *rid) {
  char *data = make_key(record);
  RC rc = index_handler_.delete_entry(data, rid);
  free(data);
  return rc;
}

IndexScanner *BplusTreeIndex::create_scanner(const char *left_key, int left_len, bool left_inclusive,
                                             const char *right_key, int right_len, bool right_inclusive) {
  BplusTreeIndexScanner *index_scanner = new BplusTreeIndexScanner(index_handler_);
  RC rc = index_scanner->open(left_key, left_len, left_inclusive, right_key, right_len, right_inclusive);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open index scanner. rc=%d:%s", rc, strrc(rc));
    delete index_scanner;
    return nullptr;
  }
  return index_scanner;
}

RC BplusTreeIndex::sync() { return index_handler_.sync(); }

////////////////////////////////////////////////////////////////////////////////
BplusTreeIndexScanner::BplusTreeIndexScanner(BplusTreeHandler &tree_handler) : tree_scanner_(tree_handler) {}

BplusTreeIndexScanner::~BplusTreeIndexScanner() noexcept { tree_scanner_.close(); }

RC BplusTreeIndexScanner::open(const char *left_key, int left_len, bool left_inclusive, const char *right_key,
                               int right_len, bool right_inclusive) {
  return tree_scanner_.open(left_key, left_len, left_inclusive, right_key, right_len, right_inclusive);
}

RC BplusTreeIndexScanner::next_entry(RID *rid) { return tree_scanner_.next_entry(*rid); }

RC BplusTreeIndexScanner::destroy() {
  delete this;
  return RC::SUCCESS;
}
