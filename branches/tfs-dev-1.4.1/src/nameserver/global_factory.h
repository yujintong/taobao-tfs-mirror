/*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id$
 *
 * Authors:
 *   duanfei <duanfei@taobao.com>
 *      - initial release
 *
 */
#ifndef TFS_NAMESERVER_GLOBAL_FACTORY_H_ 
#define TFS_NAMESERVER_GLOBAL_FACTORY_H_ 

#include <string>
#include <Timer.h>
#include "gc.h"
#include "ns_define.h"
#include "lease_clerk.h"
#include "common/statistics.h"

namespace tfs
{
  namespace nameserver
  {
    struct GFactory
    {
      static int initialize();
      static int wait_for_shut_down();
      static int destroy();
      static NsRuntimeGlobalInformation& get_runtime_info()
      {
        return NsRuntimeGlobalInformation::instance(); 
      }     
      static NsGlobalStatisticsInfo& get_global_info()
      {
        return NsGlobalStatisticsInfo::instance();
      } 
      static tbutil::TimerPtr& get_timer()
      {
        return timer_;
      }
      static LeaseFactory& get_lease_factory()
      {
        return LeaseFactory::instance();
      }
      static GCObjectManager& get_gc_manager()
      {
        return GCObjectManager::instance();
      }
      static common::StatManager<std::string, std::string, common::StatEntry >& get_stat_mgr()
      {
        return stat_mgr_;
      }
      static tbutil::TimerPtr timer_;
      static common::StatManager<std::string, std::string, common::StatEntry > stat_mgr_;
      static std::string tfs_ns_stat_;
      static std::string tfs_ns_stat_block_count_;
    };
  }
}

#endif


