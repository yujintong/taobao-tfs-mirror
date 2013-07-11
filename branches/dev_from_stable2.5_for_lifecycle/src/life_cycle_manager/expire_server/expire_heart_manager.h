/* * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: $
 *
 * Authors:
 *   qixiao.zs<qixiao.zs@alibaba-inc.com>
 *      - initial release
 */
#ifndef TFS_EXPIRE_SERVER_HEART_MANAGER_H_
#define TFS_EXPIRE_SERVER_HEART_MANAGER_H_

#include <tbsys.h>
#include <Monitor.h>
#include <Mutex.h>
#include <Timer.h>
#include <Shared.h>
#include <Handle.h>
#include "common/kv_rts_define.h"
#include "common/error_msg.h"

namespace tfs
{
  namespace expireserver
  {
    class ExpireHeartManager
    {
      public:
        ExpireHeartManager();
        virtual ~ExpireHeartManager();

        int initialize(uint64_t rs_ipport_id, uint64_t ms_ipport_id, int64_t start_time);
        void destroy();

      private:
        void run_heart(void);
        int keepalive(void);
      private:
      class ESHeartBeatThreadHelper: public tbutil::Thread
      {
        public:
          explicit ESHeartBeatThreadHelper(ExpireHeartManager& manager): manager_(manager){}
          virtual ~ESHeartBeatThreadHelper(){};
          void run();
        private:
          ExpireHeartManager& manager_;
          DISALLOW_COPY_AND_ASSIGN(ESHeartBeatThreadHelper);
      };
        typedef tbutil::Handle<ESHeartBeatThreadHelper> ESHeartBeatThreadHelperPtr;
      private:
        DISALLOW_COPY_AND_ASSIGN(ExpireHeartManager);
        static const int8_t MAX_RETRY_COUNT;
        static const int16_t MAX_TIMEOUT_MS;
        ESHeartBeatThreadHelperPtr heart_thread_;

        bool destroy_;
        uint64_t ers_ipport_id_;
        uint64_t es_ipport_id_;
        int64_t start_time_;
        int64_t heart_inter_;

    };
  }/** namemetaserver **/
}/** tfs **/

#endif

