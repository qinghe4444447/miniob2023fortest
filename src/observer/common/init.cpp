/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */



#include "common/init.h"

#include "common/conf/ini.h"
#include "common/ini_setting.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "common/os/path.h"
#include "common/os/pidfile.h"
#include "common/os/process.h"
#include "common/os/signal.h"
#include "common/seda/init.h"
#include "common/seda/stage_factory.h"
#include "global_context.h"
#include "session/session.h"
#include "session/session_stage.h"
#include "sql/executor/execute_stage.h"
#include "sql/optimizer/optimize_stage.h"
#include "sql/parser/parse_stage.h"
#include "sql/parser/resolve_stage.h"
#include "sql/plan_cache/plan_cache_stage.h"
#include "sql/query_cache/query_cache_stage.h"
#include "storage/buffer/disk_buffer_pool.h"
#include "storage/default/default_handler.h"
#include "storage/trx/trx.h"

using namespace common;

bool *&_get_init() {
  static bool util_init = false;
  static bool *util_init_p = &util_init;
  return util_init_p;
}

bool get_init() { return *_get_init(); }

void set_init(bool value) {
  *_get_init() = value;
  return;
}

void sig_handler(int sig) {
  // Signal handler will be add in the next step.
  //  Add action to shutdown

  LOG_INFO("Receive one signal of %d.", sig);

  return;
}

int init_log(ProcessParam *process_cfg, Ini &properties) {
  const std::string &proc_name = process_cfg->get_process_name();
  try {
    // we had better alloc one lock to do so, but simplify the logic
    if (g_log) {
      return 0;
    }

    auto log_context_getter = []() { return reinterpret_cast<intptr_t>(Session::current_session()); };

    const std::string log_section_name = "LOG";
    std::map<std::string, std::string> log_section = properties.get(log_section_name);

    std::string log_file_name;

    // get log file name
    std::string key = "LOG_FILE_NAME";
    std::map<std::string, std::string>::iterator it = log_section.find(key);
    if (it == log_section.end()) {
      log_file_name = proc_name + ".log";
      std::cout << "Not set log file name, use default " << log_file_name << std::endl;
    } else {
      log_file_name = it->second;
    }

    log_file_name = getAboslutPath(log_file_name.c_str());

    LOG_LEVEL log_level = LOG_LEVEL_INFO;
    key = ("LOG_FILE_LEVEL");
    it = log_section.find(key);
    if (it != log_section.end()) {
      int log = (int)log_level;
      str_to_val(it->second, log);
      log_level = (LOG_LEVEL)log;
    }

    LOG_LEVEL console_level = LOG_LEVEL_INFO;
    key = ("LOG_CONSOLE_LEVEL");
    it = log_section.find(key);
    if (it != log_section.end()) {
      int log = (int)console_level;
      str_to_val(it->second, log);
      console_level = (LOG_LEVEL)log;
    }

    LoggerFactory::init_default(log_file_name, log_level, console_level);
    g_log->set_context_getter(log_context_getter);

    key = ("DefaultLogModules");
    it = log_section.find(key);
    if (it != log_section.end()) {
      g_log->set_default_module(it->second);
    }

    if (process_cfg->is_demon()) {
      sys_log_redirect(log_file_name.c_str(), log_file_name.c_str());
    }

    return 0;
  } catch (std::exception &e) {
    std::cerr << "Failed to init log for " << proc_name << SYS_OUTPUT_FILE_POS << SYS_OUTPUT_ERROR << std::endl;
    return errno;
  }

  return 0;
}

void cleanup_log() {

  if (g_log) {
    delete g_log;
    g_log = nullptr;
  }
  return;
}

int prepare_init_seda() {
  static StageFactory session_stage_factory("SessionStage", &SessionStage::make_stage);
  return 0;
}

int init_global_objects(ProcessParam *process_param, Ini &properties) {
  GCTX.buffer_pool_manager_ = new BufferPoolManager();
  BufferPoolManager::set_instance(GCTX.buffer_pool_manager_);

  GCTX.handler_ = new DefaultHandler();

  DefaultHandler::set_default(GCTX.handler_);

  int ret = 0;
  RC rc = TrxKit::init_global(process_param->trx_kit_name().c_str());
  if (rc != RC::SUCCESS) {
    LOG_ERROR("failed to init trx kit. rc=%s", strrc(rc));
    ret = -1;
  }
  GCTX.trx_kit_ = TrxKit::instance();

  rc = GCTX.handler_->init("miniob");
  if (OB_FAIL(rc)) {
    LOG_ERROR("failed to init handler. rc=%s", strrc(rc));
    return -1;
  }
  return ret;
}

int uninit_global_objects() {
  // TODO use global context
  DefaultHandler *default_handler = &DefaultHandler::get_default();
  if (default_handler != nullptr) {
    DefaultHandler::set_default(nullptr);
    delete default_handler;
  }

  BufferPoolManager *bpm = &BufferPoolManager::instance();
  if (bpm != nullptr) {
    BufferPoolManager::set_instance(nullptr);
    delete bpm;
  }
  return 0;
}

int init(ProcessParam *process_param) {
  if (get_init()) {

    return 0;
  }

  set_init(true);

  // Run as daemon if daemonization requested
  int rc = STATUS_SUCCESS;
  if (process_param->is_demon()) {
    rc = daemonize_service(process_param->get_std_out().c_str(), process_param->get_std_err().c_str());
    if (rc != 0) {
      std::cerr << "Shutdown due to failed to daemon current process!" << std::endl;
      return rc;
    }
  }

  writePidFile(process_param->get_process_name().c_str());

  // Initialize global variables before enter multi-thread mode
  // to avoid race condition

  // Read Configuration files
  rc = get_properties()->load(process_param->get_conf());
  if (rc) {
    std::cerr << "Failed to load configuration files" << std::endl;
    return rc;
  }

  // Init tracer
  rc = init_log(process_param, *get_properties());
  if (rc) {
    std::cerr << "Failed to init Log" << std::endl;
    return rc;
  }

  std::string conf_data;
  get_properties()->to_string(conf_data);
  LOG_INFO("Output configuration \n%s", conf_data.c_str());

  rc = init_global_objects(process_param, *get_properties());
  if (rc != 0) {
    LOG_ERROR("failed to init global objects");
    return rc;
  }

  // seda is used for backend async event handler
  // the latency of seda is slow, it isn't used for critical latency
  // environment.
  prepare_init_seda();
  rc = init_seda(process_param);
  if (rc) {
    LOG_ERROR("Failed to init seda configuration!");
    return rc;
  }

  // Block interrupt signals before creating child threads.
  // setSignalHandler(sig_handler);
  // sigset_t newSigset, oset;
  // blockDefaultSignals(&newSigset, &oset);
  //  wait interrupt signals
  // startWaitForSignals(&newSigset);

  LOG_INFO("Successfully init utility");

  return STATUS_SUCCESS;
}

void cleanup_util() {
  uninit_global_objects();

  if (nullptr != get_properties()) {
    delete get_properties();
    get_properties() = nullptr;
  }

  LOG_INFO("Shutdown Cleanly!");

  // Finalize tracer
  cleanup_log();

  set_init(false);
  return;
}

void cleanup() { cleanup_util(); }
