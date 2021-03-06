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
 *   duolong <duolong@taobao.com>
 *      - initial release
 *   qushan<qushan@taobao.com> 
 *      - modify 2009-03-27
 *   duanfei <duanfei@taobao.com> 
 *      - modify 2010-04-23
 *
 */
#ifndef TFS_NAMESERVER_OPLOG_H_
#define TFS_NAMESERVER_OPLOG_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <ext/hash_map>
#include <errno.h>
#include <dirent.h>
#include <Mutex.h>

#include "ns_define.h"
#include "common/file_queue.h"

namespace tfs
{
  namespace nameserver
  {
    enum OpLogType
    {
      OPLOG_TYPE_BLOCK_OP = 0x00,
      OPLOG_TYPE_REPLICATE_MSG,
      OPLOG_TYPE_COMPACT_MSG
    };
#pragma pack(1)
    struct OpLogHeader
    {
      uint32_t seqno_;
      uint32_t time_;
      uint32_t crc_;
      uint16_t length_;
      int8_t  type_;
      int8_t  reserve_;
      char  data_[0];
    };
    struct OpLogRotateHeader
    {
      int32_t rotate_seqno_;
      int32_t rotate_offset_;
    };
#pragma pack()
 
    struct BlockOpLog
    {
      common::BlockInfo info_;
      common::VUINT32 blocks_;
      common::VUINT64 servers_;
      int8_t cmd_;
      int serialize(char* buf, const int64_t buf_len, int64_t& pos) const;
      int deserialize(const char* buf, const int64_t data_len, int64_t& pos);
      int64_t get_serialize_size(void) const;
      void dump(void) const;
    };

   class OpLog
    {
    public:
      explicit OpLog(const std::string& path, int maxLogSlotsSize = 0x400);
      virtual ~OpLog();
      int initialize();
      int update_oplog_rotate_header(const OpLogRotateHeader& head);
      bool finish(time_t now, bool force = false) const;
      int write(uint8_t type, const char* const data, const int32_t length);
      inline void reset(time_t t = time(NULL))
      {
        last_flush_time_ = t;
        slots_offset_ = 0;
      }
      inline const char* const get_buffer() const
      {
        return buffer_;
      }
      inline int32_t get_slots_offset() const
      {
        return slots_offset_;
      }
      inline const OpLogRotateHeader* get_oplog_rotate_header() const
      {
        return &oplog_rotate_header_;
      }
    public:
      static int const MAX_LOG_SIZE = sizeof(OpLogHeader) + common::BLOCKINFO_SIZE + 1 + 64 * common::INT64_SIZE;
      const int MAX_LOG_SLOTS_SIZE;
      const int MAX_LOG_BUFFER_SIZE;
    private:
      tbutil::Mutex mutex_;
      OpLogRotateHeader oplog_rotate_header_;
      std::string path_;
      uint64_t seqno_;
      time_t last_flush_time_;
      int32_t slots_offset_;
      int32_t fd_;
      char* buffer_;
    private:
      DISALLOW_COPY_AND_ASSIGN( OpLog);
      OpLog();
    };
  }//end namespace nameserver
}//end namespace tfs
#endif
