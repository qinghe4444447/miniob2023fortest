/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */



#include "storage/common/meta_util.h"
#include "common/defs.h"

std::string table_meta_file(const char *base_dir, const char *table_name) {
  return std::string(base_dir) + common::FILE_PATH_SPLIT_STR + table_name + TABLE_META_SUFFIX;
}

std::string view_meta_file(const char *base_dir, const char *view_name) {
  return std::string(base_dir) + common::FILE_PATH_SPLIT_STR + view_name + VIEW_META_SUFFIX;
}

std::string table_data_file(const char *base_dir, const char *table_name) {
  return std::string(base_dir) + common::FILE_PATH_SPLIT_STR + table_name + TABLE_DATA_SUFFIX;
}

std::string table_text_file(const char *base_dir, const char *table_name) {
  return std::string(base_dir) + common::FILE_PATH_SPLIT_STR + table_name + TABLE_TEXT_SUFFIX;
}

std::string table_index_file(const char *base_dir, const char *table_name, const char *index_name) {
  return std::string(base_dir) + common::FILE_PATH_SPLIT_STR + table_name + "-" + index_name + TABLE_INDEX_SUFFIX;
}
