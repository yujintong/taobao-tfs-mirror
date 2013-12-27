/*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Authors:
 *  linqing <linqing.zyd@taobao.com>
 *      - initial release
 *
 */
#ifndef TFS_REQUESTER_NSREQUESTER_H_
#define TFS_REQUESTER_NSREQUESTER_H_

#include "common/internal.h"

namespace tfs
{
  namespace requester
  {
    static const int32_t META_FLAG_ABNORMAL = -9800;
    static const int32_t FILE_STATUS_ERROR  = -9801;
    static const int32_t SYNC_SUCCESS = 0;
    static const int32_t SYNC_FAILED  = -99999;
    static const int32_t SYNC_NOTHING = -100000;
    struct CompareFileInfoByFileId
    {
      bool operator()(const common::FileInfo& left, const common::FileInfo& right) const
      {
        return left.id_ < right.id_;
      }
    };

    struct CompareFileInfoV2ByFileId
    {
      bool operator()(const common::FileInfoV2& left, const common::FileInfoV2& right) const
      {
        return left.id_ < right.id_;
      }
    };


    // all requests are sent to nameserver
    class NsRequester
    {
      public:
        // get a block's replica info, return a dataserver list
        static int get_block_replicas(const uint64_t ns_id,
            const uint64_t block_id, common::VUINT64& replicas);

        static int get_block_replicas(const uint64_t ns_id,
            const uint64_t block_id, common::BlockMeta& meta);

        // get a cluster's cluster id from nameserver
        static int get_cluster_id(const uint64_t ns_id, int32_t& cluster_id);

        // get a cluster's group count from nameserver
        static int get_group_count(const uint64_t ns_id, int32_t& group_count);

        // get a cluster's group seq from nameserver
        static int get_group_seq(const uint64_t ns_id, int32_t& group_seq);

        // get nameserver's config parameter value
        static int get_ns_param(const uint64_t ns_id, const std::string& key, std::string& value);

        // get all dataserver's ip:port from nameserver
        static int get_ds_list(const uint64_t ns_id, common::VUINT64& ds_list);

        static int read_file_infos(const std::string& ns_addr, const uint64_t block, std::multiset<std::string>& files, const int32_t version);
        static int read_file_infos(const std::string& ns_addr, const uint64_t block, std::set<common::FileInfo, CompareFileInfoByFileId>& files, const int32_t version);
        static int read_file_info(const std::string& ns_addr, const std::string& filename, common::FileInfo& info);
        static int read_file_info_v2(const std::string& ns_addr, const std::string& filename, common::FileInfoV2& info);
        static int read_file_stat(const std::string& ns_addr, const std::string& filename, common::TfsFileStat& info);
        static int read_file_real_crc(const std::string& ns_addr, const std::string& filename, uint32_t& crc);
        static int read_file_real_crc_v2(const std::string& ns_addr, const std::string& filename, common::FileInfoV2& info, const bool force);
        static int write_file(const std::string& ns_addr, const std::string& filename, const char* data, const int32_t size, const int32_t status);
        static int copy_file(const std::string& src_ns_addr, const std::string& dest_ns_addr, const std::string& file_name, const int32_t status);
        static int cmp_and_sync_file(const std::string& src_ns_addr, const std::string& dest_ns_addr, const std::string& file_name,
        const int64_t timestamp, const bool force, common::FileInfoV2& left, common::FileInfoV2& right);
        static int sync_file(const std::string& saddr, const std::string& daddr, const std::string& filename,
            const common::FileInfoV2& sfinfo, const common::FileInfoV2& dfinfo, const int64_t timestamp, const bool force);
        static int remove_block(const uint64_t block, const std::string& addr, const int32_t flag);

    private:
        static int get_block_replicas_ex(const uint64_t ns_id,
            const uint64_t block_id, const int32_t flag, common::BlockMeta& meta);
    };

    class ServerStat
    {
      public:
        ServerStat();
        virtual ~ServerStat();

        int deserialize(tbnet::DataBuffer& input, const int32_t length, int32_t& offset);
        uint64_t id_;
        int64_t use_capacity_;
        int64_t total_capacity_;
        common::Throughput total_tp_;
        common::Throughput last_tp_;
        int32_t current_load_;
        int32_t block_count_;
        time_t last_update_time_;
        time_t startup_time_;
        time_t current_time_;
        common::DataServerLiveStatus status_;
   };

  }
}

#endif
