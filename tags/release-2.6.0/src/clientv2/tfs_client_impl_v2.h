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
#ifndef TFS_CLIENTV2_TFSCLIENTIMPLV2_H_
#define TFS_CLIENTV2_TFSCLIENTIMPLV2_H_

#include <Mutex.h>
#include <stdio.h>
#include <pthread.h>

#include "common/internal.h"

namespace tfs
{
  namespace common
  {
    class BasePacketFactory;
    class BasePacketStreamer;
  }
  namespace clientv2
  {
    class tbutil::Mutex;
    class TfsFile;
    typedef std::map<int, TfsFile*> FILE_MAP;

    class TfsClientImplV2
    {
    public:
      static TfsClientImplV2* Instance()
      {
        static TfsClientImplV2 tfs_client_impl;
        return &tfs_client_impl;
      }

      int initialize(const char* ns_addr = NULL);
      int destroy();
      int set_option_flag(const int fd, const int opt_flag);
      void set_log_level(const char* level);
      void set_log_file(const char* file);

      int open(const char* file_name, const char* suffix, const char* ns_addr, const int mode);
      int open(const uint64_t block_id, const uint64_t file_id, const char* ns_addr, const int mode);
      int64_t read(const int fd, void* buf, const int64_t count);
      int64_t readv2(const int fd, void* buf, const int64_t count, common::TfsFileStat* file_info);
      int64_t write(const int fd, const void* buf, const int64_t count);
      int64_t lseek(const int fd, const int64_t offset, const int whence);
      int64_t pread(const int fd, void* buf, const int64_t count, const int64_t offset);
      int64_t pwrite(const int fd, const void* buf, const int64_t count, const int64_t offset);
      int fstat(const int fd, common::TfsFileStat* buf);
      int close(const int fd, char* ret_tfs_name = NULL, const int32_t ret_tfs_name_len = 0);
      int unlink(int64_t& file_size, const int fd, const common::TfsUnlinkType action = common::DELETE);

      int stat_file(common::TfsFileStat* file_stat, const char* file_name, const char* suffix = NULL,
          const common::TfsStatType stat_type = common::NORMAL_STAT, const char* ns_addr = NULL);
      int64_t save_file(char* ret_tfs_name, const int32_t ret_tfs_name_len,
          const char* local_file, const int32_t mode, const char* suffix = NULL,
          const char* ns_addr = NULL);
      int64_t save_file_update(const char* local_file, const int32_t flag,
        const char* tfs_name, const char* suffix, const char* ns_addr = NULL);
      int fetch_file(const char* local_file, const char* file_name, const char* suffix = NULL,
          const common::ReadDataOptionFlag read_flag = common::READ_DATA_OPTION_FLAG_NORMAL,
          const char* ns_addr = NULL);
      int unlink(int64_t& file_size, const char* file_name, const char* suffix = NULL,
          const common::TfsUnlinkType action = common::DELETE,
          const char* ns_addr = NULL);

      int64_t get_server_id();
      int32_t get_cluster_id();

    private:
      int get_fd();
      TfsFile* get_file(const int fd);
      int insert_file(const int fd, TfsFile* tfs_file);
      int erase_file(const int fd);
      int initialize_cluster_id();
      uint64_t ipport_to_addr(const char* addr);
      int64_t save_file_ex(char* ret_tfs_name, const int32_t ret_tfs_name_len, const char* local_file,
          const int32_t mode, const char* tfs_name, const char* suffix, const char* ns_addr);

   private:
      TfsClientImplV2();
      DISALLOW_COPY_AND_ASSIGN(TfsClientImplV2);
      ~TfsClientImplV2();

      bool is_init_;
      int fd_;
      uint64_t ns_addr_;
      int32_t cluster_id_;
      FILE_MAP tfs_file_map_;
      tbutil::Mutex mutex_;
      common::BasePacketFactory* packet_factory_;
      common::BasePacketStreamer* packet_streamer_;
    };
  }
}

#endif  // TFS_CLIENT_TFSCLIENTAPI_H_