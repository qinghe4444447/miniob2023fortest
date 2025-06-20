/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */



#include "storage/db/db.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include "common/defs.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "common/os/path.h"
#include "common/rc.h"
#include "storage/clog/clog.h"
#include "storage/common/meta_util.h"
#include "storage/field/field.h"
#include "storage/table/table.h"
#include "storage/table/table_meta.h"
#include "storage/trx/trx.h"
#include "storage/view/view.h"

Db::~Db() {
  for (auto &iter : opened_tables_) {
    delete iter.second;
  }
  LOG_INFO("Db has been closed: %s", name_.c_str());
}

RC Db::init(const char *name, const char *dbpath) {
  if (common::is_blank(name)) {
    LOG_ERROR("Failed to init DB, name cannot be empty");
    return RC::INVALID_ARGUMENT;
  }

  if (!common::is_directory(dbpath)) {
    LOG_ERROR("Failed to init DB, path is not a directory: %s", dbpath);
    return RC::INTERNAL;
  }

  clog_manager_.reset(new CLogManager());
  if (clog_manager_ == nullptr) {
    LOG_ERROR("Failed to init CLogManager.");
    return RC::NOMEM;
  }

  RC rc = clog_manager_->init(dbpath);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to init clog manager. dbpath=%s, rc=%s", dbpath, strrc(rc));
    return rc;
  }

  name_ = name;
  path_ = dbpath;

  rc = open_all_tables();
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to open all tables. dbpath=%s, rc=%s", dbpath, strrc(rc));
    return rc;
  }

  rc = open_all_views();
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to open all views. dbpath=%s, rc=%s", dbpath, strrc(rc));
    return rc;
  }

  rc = recover();
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to recover db. dbpath=%s, rc=%s", dbpath, strrc(rc));
    return rc;
  }
  return rc;
}

RC Db::create_table(const char *table_name, int attribute_count, const AttrInfoSqlNode *attributes) {
  RC rc = RC::SUCCESS;
  // check table_name
  if (opened_tables_.count(table_name) != 0) {
    LOG_WARN("%s has been opened before.", table_name);
    return RC::SCHEMA_TABLE_EXIST;
  }

  // 文件路径可以移到Table模块
  std::string table_file_path = table_meta_file(path_.c_str(), table_name);
  Table *table = new Table();
  int32_t table_id = next_table_id_++;
  rc = table->create(table_id, table_file_path.c_str(), table_name, path_.c_str(), attribute_count, attributes);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to create table %s.", table_name);
    delete table;
    return rc;
  }

  opened_tables_[table_name] = table;
  LOG_INFO("Create table success. table name=%s, table_id:%d", table_name, table_id);
  return RC::SUCCESS;
}

RC Db::drop_table(Trx *trx, const char *table_name) {
  // 找到所有索引 把他们删了
  RC rc = RC::SUCCESS;
  auto it = opened_tables_.find(table_name);
  if (it == opened_tables_.end()) {
    LOG_WARN("%s has not been opened before.", table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  Table *table = it->second;
  if (it->second->view()) {
    LOG_WARN("view should not be dropped", table_name);
    return RC::INVALID_ARGUMENT;
  }
  table->drop_all_indexes();
  // 这里会释放所有page
  delete table;
  opened_tables_.erase(it);
  auto table_file_name = table_data_file(path_.c_str(), table_name);
  auto table_meta_name = table_meta_file(path_.c_str(), table_name);
  auto table_text_name = table_text_file(path_.c_str(), table_name);
  if (unlink(table_file_name.c_str()) == -1) {
    LOG_ERROR("Failed to delete table (%s) data file %s.", table_name, table_file_name.c_str());
    return RC::IOERR_UNLINK;
  }
  if (unlink(table_meta_name.c_str()) == -1) {
    LOG_ERROR("Failed to delete table (%s) data meta file %s.", table_name, table_meta_name.c_str());
    return RC::IOERR_UNLINK;
  }
  if (unlink(table_text_name.c_str()) == -1) {
    LOG_ERROR("Failed to delete table (%s) data text file %s.", table_name, table_text_name.c_str());
    return RC::IOERR_UNLINK;
  }
  return RC::SUCCESS;
}

Table *Db::find_table(const char *table_name) const {
  std::unordered_map<std::string, Table *>::const_iterator iter = opened_tables_.find(table_name);
  if (iter != opened_tables_.end()) {
    return iter->second;
  }
  return nullptr;
}

Table *Db::find_table(int32_t table_id) const {
  for (auto pair : opened_tables_) {
    if (pair.second->table_id() == table_id) {
      return pair.second;
    }
  }
  return nullptr;
}

RC Db::open_all_tables() {
  std::vector<std::string> table_meta_files;
  int ret = common::list_file(path_.c_str(), TABLE_META_FILE_PATTERN, table_meta_files);
  if (ret < 0) {
    LOG_ERROR("Failed to list table meta files under %s.", path_.c_str());
    return RC::IOERR_READ;
  }

  RC rc = RC::SUCCESS;
  for (const std::string &filename : table_meta_files) {
    Table *table = new Table();
    rc = table->open(filename.c_str(), path_.c_str());
    if (rc != RC::SUCCESS) {
      delete table;
      LOG_ERROR("Failed to open table. filename=%s", filename.c_str());
      return rc;
    }

    if (opened_tables_.count(table->name()) != 0) {
      delete table;
      LOG_ERROR("Duplicate table with difference file name. table=%s, the other filename=%s", table->name(),
                filename.c_str());
      return RC::INTERNAL;
    }

    if (table->table_id() >= next_table_id_) {
      next_table_id_ = table->table_id() + 1;
    }
    opened_tables_[table->name()] = table;
    LOG_INFO("Open table: %s, file: %s", table->name(), filename.c_str());
  }

  LOG_INFO("All table have been opened. num=%d", opened_tables_.size());
  return rc;
}

RC Db::open_all_views() {
  std::vector<std::string> view_meta_files;
  int ret = common::list_file(path_.c_str(), VIEW_META_FILE_PATTERN, view_meta_files);
  if (ret < 0) {
    LOG_ERROR("Failed to list view meta files under %s.", path_.c_str());
    return RC::IOERR_READ;
  }

  RC rc = RC::SUCCESS;
  // 首先把所有view都找到，不要parse sql
  for (const std::string &filename : view_meta_files) {
    View *view = new View;
    auto view_file_path = path_ + common::FILE_PATH_SPLIT_STR + filename;
    rc = view->open_without_parse(view_file_path.c_str());
    if (rc != RC::SUCCESS) {
      delete view;
      LOG_ERROR("Failed to open view. filename=%s", filename.c_str());
      return rc;
    }
    if (opened_tables_.count(view->name())) {
      delete view;
      LOG_ERROR("Duplicate table with difference file name. table=%s, the other filename=%s", view->name().c_str(),
                filename.c_str());
      return RC::INTERNAL;
    }
    opened_tables_[view->name()] = new Table(view);
    LOG_INFO("Open view: %s, file: %s", view->name().c_str(), filename.c_str());
  }
  // parse sql，将所有view初始化完成
  for (auto &[_, table] : opened_tables_) {
    if (table->view()) {
      auto *view = table->view();
      rc = view->parse_sql(this);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to parse view sql, name =%s", view->name().c_str());
        return rc;
      }
    }
  }
  return rc;
}

const char *Db::name() const { return name_.c_str(); }

void Db::all_tables(std::vector<std::string> &table_names) const {
  for (const auto &table_item : opened_tables_) {
    table_names.emplace_back(table_item.first);
  }
}

RC Db::sync() {
  RC rc = RC::SUCCESS;
  for (const auto &table_pair : opened_tables_) {
    Table *table = table_pair.second;
    rc = table->sync();
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to flush table. table=%s.%s, rc=%d:%s", name_.c_str(), table->name(), rc, strrc(rc));
      return rc;
    }
    LOG_INFO("Successfully sync table db:%s, table:%s.", name_.c_str(), table->name());
  }
  LOG_INFO("Successfully sync db. db=%s", name_.c_str());
  return rc;
}

RC Db::recover() { return clog_manager_->recover(this); }

CLogManager *Db::clog_manager() { return clog_manager_.get(); }

RC Db::create_view(const char *view_name, const char *sql, SelectStmt *select, std::vector<std::string> &names) {
  if (opened_tables_.count(view_name)) {
    return RC::SCHEMA_VIEW_EXIST;
  }
  View *view = new View;
  RC rc = RC::SUCCESS;
  auto view_file_path = view_meta_file(path_.c_str(), view_name);
  rc = view->create_view(this, view_file_path.c_str(), view_name, select, names);
  if (rc != RC::SUCCESS)
    return rc;
  opened_tables_[view_name] = new Table(view);
  return RC::SUCCESS;
}

View *Db::find_view(const char *view_name) const {
  auto it = opened_tables_.find(view_name);
  if (it == opened_tables_.end()) {
    return nullptr;
  }
  return it->second->view();
}
