/*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Authors:
 *   linqing <linqing.zyd@taobao.com>
 *      - initial release
 *
 */
#ifndef TFS_DATASERVER_CHECKMANAGER_H_
#define TFS_DATASERVER_CHECKMANAGER_H_

#include "common/error_msg.h"
#include "common/internal.h"
#include "common/array_helper.h"

namespace tfs
{
  namespace dataserver
  {
    class BlockManager;
    class DataService;
    class CheckManager
    {
      public:
        CheckManager(DataService& service);
        ~CheckManager();
        BlockManager& get_block_manager();

        void run_check();
        int handle(tbnet::Packet* packet);

      private:
        int get_check_blocks(CheckBlockRequestMessage* message);
        int add_check_blocks(ReportCheckBlockMessage* message);
        int report_check_blocks();
        int check_block(const uint64_t block_id);
        void compare_block_fileinfos(std::vector<common::FileInfoV2>& left,
            std::vector<common::FileInfoV2>& right, common::VUINT64& more, common::VUINT64& less);

     private:
        DataService& service_;
        std::deque<uint64_t> pending_blocks_;
        std::vector<uint64_t> success_blocks_;
        tbutil::Mutex mutex_;
        int64_t seqno_;
        int64_t check_server_id_;
    };
  }
}
#endif
