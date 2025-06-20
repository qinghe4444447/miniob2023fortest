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
#include "event/session_event.h"
#include "event/sql_event.h"
#include "session/session.h"
#include "sql/executor/sql_result.h"
#include "sql/operator/string_list_physical_operator.h"
#include "storage/trx/trx.h"

/**
 * @brief 事务开始语句的执行器
 * @ingroup Executor
 */
class TrxBeginExecutor {
public:
  TrxBeginExecutor() = default;
  virtual ~TrxBeginExecutor() = default;

  RC execute(SQLStageEvent *sql_event) {
    SessionEvent *session_event = sql_event->session_event();

    Session *session = session_event->session();
    Trx *trx = session->current_trx();

    session->set_trx_multi_operation_mode(true);

    return trx->start_if_need();
  }
};