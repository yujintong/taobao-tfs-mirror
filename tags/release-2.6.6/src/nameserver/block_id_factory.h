/*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: block_id_factory.h 2140 2011-03-29 01:42:04Z duanfei $
 *
 * Authors:
 *   duanfei <duanfei@taobao.com>
 *      - initial release
 *
 */
#ifndef TFS_NAMESERVER_FSIMAGE_H
#define TFS_NAMESERVER_FSIMAGE_H

#include <Mutex.h>
#include "ns_define.h"

namespace tfs
{
  namespace nameserver
  {
    class OpLogSyncManager;
    class BlockIdFactory
    {
      public:
        BlockIdFactory(OpLogSyncManager& manager);
        virtual ~BlockIdFactory();
        int initialize(const std::string& path);
        int destroy();
        uint64_t generation(const bool verify);
        int update(const uint64_t id);
        uint64_t skip(const int32_t num = SKIP_BLOCK_NUMBER);
        uint64_t get() const;
      private:
        int flush_(const uint64_t id) const;
        DISALLOW_COPY_AND_ASSIGN(BlockIdFactory);
        OpLogSyncManager& manager_;
        tbutil::Mutex mutex_;
        static const uint16_t BLOCK_START_NUMBER;
        static const uint32_t SKIP_BLOCK_NUMBER;
        static const uint16_t FLUSH_BLOCK_NUMBER;
        static const uint64_t MAX_BLOCK_ID;
        uint64_t global_id_;
        uint64_t last_flush_id_;
        int32_t  fd_;
    };
  }/** nameserver **/
}/** tfs **/

#endif
