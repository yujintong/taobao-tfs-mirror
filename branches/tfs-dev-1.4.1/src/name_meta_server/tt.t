database_helper.cpp                                                                                 0000644 0141723 0001014 00000002323 11613721322 013607  0                                                                                                    ustar   daoan                           daoan                                                                                                                                                                                                                  /*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id
 *
 *
 *   Authors:
 *          daoan(daoan@taobao.com)
 *
 */

#include "database_helper.h"
#include "common/internal.h"
namespace tfs
{
  using namespace common;
  namespace namemetaserver
  {
    DatabaseHelper::DatabaseHelper()
    {
      conn_str_[0] = '\0';
      user_name_[0] = '\0';
      passwd_[0] = '\0';
      is_connected_ = false;
    }

    DatabaseHelper::~DatabaseHelper()
    {
    }

    int DatabaseHelper::set_conn_param(const char* conn_str, const char* user_name, const char* passwd)
    {
      int ret = TFS_SUCCESS;
      if (NULL == conn_str || NULL == user_name || NULL == passwd)
      {
        ret = TFS_ERROR;
      }
      else
      {
        snprintf(conn_str_, CONN_STR_LEN, "%s", conn_str);
        snprintf(user_name_, CONN_STR_LEN, "%s", user_name);
        snprintf(passwd_, CONN_STR_LEN, "%s", passwd);
      }
      return ret;
    }

    bool DatabaseHelper::is_connected() const
    {
      return is_connected_;
    }

  }
}
                                                                                                                                                                                                                                                                                                             database_pool.cpp                                                                                   0000644 0141723 0001014 00000005607 11613721322 013311  0                                                                                                    ustar   daoan                           daoan                                                                                                                                                                                                                  /*
* (C) 2007-2010 Alibaba Group Holding Limited.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
*
* Version: $Id
*
* Authors:
*   daoan <daoan@taobao.com>
*      - initial release
*
*/
#include "database_pool.h"
#include "mysql_database_helper.h"
#include "common/internal.h"
namespace tfs
{
  using namespace common;
  namespace namemetaserver
  {
    DataBasePool::DataBasePool(const int32_t pool_size)
    {
      my_init();
      pool_size_ = pool_size;
      if (pool_size_ > MAX_POOL_SIZE)
      {
        pool_size_ = MAX_POOL_SIZE;
      } else if (pool_size_ < 1)
      {
        pool_size_ = 1;
      }
    }
    DataBasePool::~DataBasePool()
    {
      for (int i = 0; i < MAX_POOL_SIZE; i++)
      {
        if (NULL != base_info_[i].database_helper_)
        {
          if (base_info_[i].busy_flag_)
          {
            TBSYS_LOG(ERROR, "release not match with get");
          }
          delete base_info_[i].database_helper_;
          base_info_[i].database_helper_ = NULL;
        }
      }
      mysql_thread_end();
    }
    bool DataBasePool::init_pool(const char** conn_str, const char** user_name,
        const char** passwd, const int32_t* hash_flag)
    {
      bool ret = true;
      for (int i = 0; i < pool_size_; i++)
      {
        if (NULL != base_info_[i].database_helper_)
        {
          delete base_info_[i].database_helper_;
        }
        base_info_[i].database_helper_ = new MysqlDatabaseHelper();
        if (TFS_SUCCESS != base_info_[i].database_helper_->set_conn_param(conn_str[i], user_name[i], passwd[i]))
        {
          ret = false;
          break;
        }
        base_info_[i].busy_flag_ = false;
        base_info_[i].hash_flag_ = hash_flag[i];
      }
      if (!ret)
      {
        for (int i = 0; i < pool_size_; i++)
        {
          base_info_[i].busy_flag_ = true;
        }
      }
      return ret;
    }
    DatabaseHelper* DataBasePool::get(const int32_t hash_flag)
    {
      DatabaseHelper* ret = NULL;
      tbutil::Mutex::Lock lock(mutex_);
      for (int i = 0; i < pool_size_; i++)
      {
        if (!base_info_[i].busy_flag_ && base_info_[i].hash_flag_ == hash_flag)
        {
          ret = base_info_[i].database_helper_;
          base_info_[i].busy_flag_ = true;
          break;
        }
      }
      return ret;
    }
    void DataBasePool::release(DatabaseHelper* database_helper)
    {
      tbutil::Mutex::Lock lock(mutex_);
      for (int i = 0; i < pool_size_; i++)
      {
        if (base_info_[i].database_helper_ == database_helper)
        {
          if (base_info_[i].busy_flag_)
          {
            base_info_[i].busy_flag_ = false;
          }
          else
          {
            TBSYS_LOG(ERROR, "some error in your code");
          }
          break;
        }
      }
    }

  }
}
                                                                                                                         main.cpp                                                                                            0000644 0141723 0001014 00000001047 11613721322 011432  0                                                                                                    ustar   daoan                           daoan                                                                                                                                                                                                                  /*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: main.cpp 495 2011-06-14 08:47:12Z nayan@taobao.com $
 *
 * Authors:
 *   nayan <nayan@taobao.com>
 *      - initial release
 *
 */
#include "meta_server_service.h"

int main(int argc, char* argv[])
{
  tfs::namemetaserver::MetaServerService service;
  return service.main(argc, argv);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         meta_server_service.cpp                                                                             0000644 0141723 0001014 00000061272 11613745346 014564  0                                                                                                    ustar   daoan                           daoan                                                                                                                                                                                                                  /*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id
 *
 * Authors:
 *   nayan <nayan@taobao.com>
 *      - initial release
 *
 */

#include "common/base_packet.h"
#include "meta_server_service.h"

using namespace tfs::common;
using namespace std;

namespace tfs
{
  namespace namemetaserver
  {
    MetaServerService::MetaServerService() : store_manager_(NULL)
    {
      store_manager_ =  new MetaStoreManager();
    }

    MetaServerService::~MetaServerService()
    {
      tbsys::gDelete(store_manager_);
    }

    tbnet::IPacketStreamer* MetaServerService::create_packet_streamer()
    {
      return new common::BasePacketStreamer();
    }

    void MetaServerService::destroy_packet_streamer(tbnet::IPacketStreamer* streamer)
    {
      tbsys::gDelete(streamer);
    }

    common::BasePacketFactory* MetaServerService::create_packet_factory()
    {
      return new message::MessageFactory();
    }

    void MetaServerService::destroy_packet_factory(common::BasePacketFactory* factory)
    {
      tbsys::gDelete(factory);
    }

    const char* MetaServerService::get_log_file_path()
    {
      return NULL;
    }

    const char* MetaServerService::get_pid_file_path()
    {
      return NULL;
    }

    int MetaServerService::initialize(int argc, char* argv[])
    {
      UNUSED(argc);
      UNUSED(argv);
      return TFS_SUCCESS;
    }

    int MetaServerService::destroy_service()
    {
      return TFS_SUCCESS;
    }

    bool MetaServerService::handlePacketQueue(tbnet::Packet *packet, void *args)
    {
      int ret = true;
      BasePacket* base_packet = NULL;
      if (!(ret = BaseService::handlePacketQueue(packet, args)))
      {
        TBSYS_LOG(ERROR, "call BaseService::handlePacketQueue fail. ret: %d", ret);
      }
      else
      {
        base_packet = dynamic_cast<BasePacket*>(packet);
        switch (base_packet->getPCode())
        {
        default:
          ret = EXIT_UNKNOWN_MSGTYPE;
          TBSYS_LOG(ERROR, "unknown msg type: %d", base_packet->getPCode());
          break;
        }
      }

      if (ret != TFS_SUCCESS && NULL != base_packet)
      {
        base_packet->reply_error_packet(TBSYS_LOG_LEVEL(ERROR), ret, "execute message failed");
      }

      // always return true. packet will be freed by caller
      return true;
    }

    int MetaServerService::create(const int64_t app_id, const int64_t uid,
                                  const char* file_path, const FileType type)
    {
      char name[MAX_FILE_PATH_LEN], pname[MAX_FILE_PATH_LEN];
      int32_t name_len = 0, pname_len = 0;
      int ret = TFS_SUCCESS;

      MetaInfo p_meta_info;
      std::vector<std::string> v_name;
      if ((ret = parse_name(file_path, v_name)) != TFS_SUCCESS)
      {
        TBSYS_LOG(WARN, "file_path(%s) is invalid", file_path);
      }
      else
      {
        int32_t depth = get_depth(v_name);

        if ((ret = get_p_meta_info(app_id, uid, v_name, p_meta_info, pname_len)) != TFS_SUCCESS)
        {
          if (depth == 1)
          {
            ret = create_top_dir(app_id, uid);
            if (TFS_SUCCESS == ret)
            {
              get_name(v_name, depth - 1, pname, MAX_FILE_PATH_LEN, pname_len);
              ret = get_p_meta_info(app_id, uid, v_name, p_meta_info, pname_len);
            }
          }
        }

        if (TFS_SUCCESS == ret) 
        {
          get_name(v_name, depth, name, MAX_FILE_PATH_LEN, name_len);
          TBSYS_LOG(DEBUG, "name: %s, ppid: %lu, pid: %lu", name, p_meta_info.pid_, p_meta_info.id_);
          if ((ret = store_manager_->insert(app_id, uid, p_meta_info.pid_, p_meta_info.name_.c_str(), pname_len,
                p_meta_info.id_, name, name_len, type)) != TFS_SUCCESS)
          {
            TBSYS_LOG(ERROR, "create fail: %s, type: %d, ret: %d", file_path, type, ret);
          }
        }
      }

      TBSYS_LOG(DEBUG, "create %s, type: %d, appid: %"PRI64_PREFIX"d, uid: %"PRI64_PREFIX"d, filepath: %s",
                TFS_SUCCESS == ret ? "success" : "fail", type, app_id, uid, file_path);

      return ret;
    }

    int MetaServerService::rm(const int64_t app_id, const int64_t uid,
                              const char* file_path, const FileType type)
    {
      char name[MAX_FILE_PATH_LEN];
      int32_t name_len = 0, pname_len = 0;
      int ret = TFS_SUCCESS;

      MetaInfo p_meta_info;
      std::vector<std::string> v_name;
      if ((ret = parse_name(file_path, v_name)) != TFS_SUCCESS)
      {
        TBSYS_LOG(WARN, "file_path(%s) is invalid", file_path);
      }
      else
      {
        int32_t depth = get_depth(v_name);

        if ((ret = get_p_meta_info(app_id, uid, v_name, p_meta_info, pname_len)) == TFS_SUCCESS)
        {
          get_name(v_name, depth, name, MAX_FILE_PATH_LEN, name_len);

          std::vector<MetaInfo> v_meta_info;
          std::vector<MetaInfo>::iterator iter;

          ret = store_manager_->select(app_id, uid, p_meta_info.id_, name, name_len, v_meta_info);
          if ((TFS_SUCCESS == ret) && (static_cast<int32_t>(v_meta_info.size()) > 0))
          {
            iter = v_meta_info.begin();
            if ((ret = store_manager_->remove(app_id, uid, 
                    p_meta_info.pid_, p_meta_info.name_.c_str(), pname_len,
                  p_meta_info.id_, (*iter).id_, name, name_len, type)) != TFS_SUCCESS)
            {
              TBSYS_LOG(ERROR, "rm fail: %s, type: %d, ret: %d", file_path, type, ret);
            }
          }
        }
      }

      TBSYS_LOG(DEBUG, "rm %s, type: %d, appid: %"PRI64_PREFIX"d, uid: %"PRI64_PREFIX"d, filepath: %s",
                TFS_SUCCESS == ret ? "success" : "fail", type, app_id, uid, file_path);

      return ret;
    }

    int MetaServerService::mv(const int64_t app_id, const int64_t uid,
                              const char* file_path, const char* dest_file_path,
                              const FileType type)
    {
      char name[MAX_FILE_PATH_LEN], dest_name[MAX_FILE_PATH_LEN];
      std::vector<std::string> v_name, dest_v_name;
      int32_t name_len = 0, pname_len = 0, dest_name_len = 0, dest_pname_len = 0;
      int ret = TFS_SUCCESS;
      MetaInfo p_meta_info, dest_p_meta_info;

      if ((ret = parse_name(file_path, v_name)) != TFS_SUCCESS)
      {
        TBSYS_LOG(WARN, "file_path(%s) is invalid", file_path);
      }
      else if ((ret = parse_name(dest_file_path, dest_v_name)) != TFS_SUCCESS)
      {
        TBSYS_LOG(WARN, "dest_file_path(%s) is invalid", dest_file_path);
      }
      else
      {
        int32_t depth = get_depth(v_name);
        int32_t dest_depth = get_depth(dest_v_name);

        if ((ret = get_p_meta_info(app_id, uid, v_name, p_meta_info, pname_len)) == TFS_SUCCESS)
        {
          if ((ret = get_p_meta_info(app_id, uid, dest_v_name, dest_p_meta_info, dest_pname_len)) == TFS_SUCCESS)
          {
            if (TFS_SUCCESS == ret) 
            {
              get_name(v_name, depth, name, MAX_FILE_PATH_LEN, name_len);
              get_name(dest_v_name, dest_depth, dest_name, MAX_FILE_PATH_LEN, dest_name_len);

              if ((ret = store_manager_->update(app_id, uid, 
                      p_meta_info.pid_, p_meta_info.id_, p_meta_info.name_.c_str(), pname_len,
                      dest_p_meta_info.pid_, dest_p_meta_info.id_, dest_p_meta_info.name_.c_str(), dest_pname_len,
                      name, name_len, dest_name, dest_name_len, type)) != TFS_SUCCESS)
                TBSYS_LOG(ERROR, "mv fail: %s, type: %d, ret: %d", file_path, type, ret);
            }
          }
        }
      }

      TBSYS_LOG(DEBUG, "mv %s, type: %d, appid: %"PRI64_PREFIX"d, uid: %"PRI64_PREFIX"d, filepath: %s",
                TFS_SUCCESS == ret ? "success" : "fail", type, app_id, uid, file_path);

      return ret;
    }

    int MetaServerService::read(const int64_t app_id, const int64_t uid, const char* file_path,
                                const int64_t offset, const int64_t size,
                                int32_t& cluster_id, vector<FragMeta>& v_frag_info, bool& still_have)
    {
      char name[MAX_FILE_PATH_LEN];
      int32_t name_len = 0, pname_len = 0;
      int ret = TFS_SUCCESS;
      still_have = false;

      MetaInfo p_meta_info;
      std::vector<MetaInfo> tmp_v_meta_info;
      std::vector<std::string> v_name;

      if ((ret = parse_name(file_path, v_name)) != TFS_SUCCESS)
      {
        TBSYS_LOG(WARN, "file_path(%s) is invalid", file_path);
      }
      else
      {
        int32_t depth = get_depth(v_name);

        if ((ret = get_p_meta_info(app_id, uid, v_name, p_meta_info, pname_len)) != TFS_SUCCESS)
        {
          TBSYS_LOG(ERROR, "get parent meta info fail. ret: %d", ret);
        }
        else
        {
          get_name(v_name, depth, name, MAX_FILE_PATH_LEN, name_len);
          ret = get_meta_info(app_id, uid, p_meta_info.id_, name, name_len, offset, tmp_v_meta_info);
          if (ret = TFS_SUCCESS)
          {
            if ((ret = read_frag_info(tmp_v_meta_info, pid, name, name_len, offset, size, 
                    cluster_id, v_frag_info, still_have)) != TFS_SUCCESS)
            {
              TBSYS_LOG(WARN, "parse read frag info fail. ret: %d", ret);
            }
          }
        }
      }
      return ret;
    }
    int MetaServerService::get_meta_info(const int64_t app_id, const int64_t uid, const int64_t pid,
        const char* name, const int32_t name_len, const int64_t offset,
        std::vector<MetaInfo>& v_meta_info);
    {
      int ret = TFS_ERROR;
      v_meta_info.clear();
      bool still_have = false;
      do {
        still_have = false;
        ret = store_manager_->select(app_id, uid, pid, 
            name, name_len, tmp_v_meta_info);
        if (TFS_SUCCESS != ret)
        {
          TBSYS_LOG(WARN, "get read meta info fail, ret: %d", ret);
          break;
        }
        if (!tmp_v_meta_info.empty())
        {
          MetaInfo last_metaInfo& = tmp_v_meta_info[tmp_v_meta_info.size() - 1];
          if (pid == last_metaInfo.pid_ && 0 == memcmp(last_metaInfo.name_.data(), name, name_len))
          {
            if (last_metaInfo.frag_info_.get_last_offset() < offset)
            {
              still_have = true;
            }
          }
        }
      } while(TFS_SUCCESS == ret && still_have);
      return ret;
    }

    int MetaServerService::write(const int64_t app_id, const int64_t uid,
        const char* file_path, const int32_t cluster_id, 
        vector<FragMeta>& v_frag_info, int32_t* write_ret)
    {
      //sort v_frag_info first
      char name[MAX_FILE_PATH_LEN], pname[MAX_FILE_PATH_LEN];
      int32_t name_len = 0, pname_len = 0;
      int ret = TFS_SUCCESS;

      MetaInfo p_meta_info;
      std::vector<MetaInfo> tmp_v_meta_info;
      std::vector<std::string> v_name;

      if ((ret = parse_name(file_path, v_name)) != TFS_SUCCESS)
      {
        TBSYS_LOG(WARN, "file_path(%s) is invalid", file_path);
      }
      else
      {
        int32_t depth = get_depth(v_name);

        if ((ret = get_p_meta_info(app_id, uid, v_name, p_meta_info, pname_len)) != TFS_SUCCESS)
        {
          TBSYS_LOG(ERROR, "get parent meta info fail. ret: %d", ret);
        }
        else
        {
          get_name(v_name, depth, name, MAX_FILE_PATH_LEN, name_len);
          int32_t retry = 3;
          do
          {
            tmp_v_meta_info.clear();
            if ((ret = store_manager_->select(app_id, uid, p_meta_info.id_, name, name_len, tmp_v_meta_info))
                != TFS_SUCCESS)
            {
              TBSYS_LOG(WARN, "get meta info fail, ret: %d", ret);
            }
            else if ((ret = write_frag_info(cluster_id, tmp_v_meta_info, v_frag_info)) != TFS_SUCCESS)
            {
              TBSYS_LOG(WARN, "merge write frag info fail.");
            }
            else if ((ret = update_write_frag_info(tmp_v_meta_info, app_id, uid, p_meta_info, pname, pname_len,
                    name, name_len)) != TFS_SUCCESS)
            {
              TBSYS_LOG(WARN, "insert new write meta info fail. ret: %d", ret);
            }
          } while (INSERT_VERSION_ERROR == ret && retry-- > 0);
        }
      }

      return ret;
    }

    int MetaServerService::read_frag_info(const vector<MetaInfo>& v_meta_info, const int64_t pid,
        const char* name, const int32_t name_len, const int64_t offset,
        const int64_t size, int32_t& cluster_id,
        vector<FragMeta>& v_out_frag_info, bool& still_have)
    {
      int64_t end_offset = offset + size;
      vector<MetaInfo>::const_iterator meta_info_it = v_meta_info.begin();;
      cluster_id = -1;
      v_out_frag_info.clear();
      still_have = false;

      while (meta_info_it != v_meta_info.end())
      {
        if (meta_info_it->pid != pid || 0 != memcpy(meta_info_it->name_.data(), name, name_len))
        {
          break;
        }
        if (meta_info_it->frag_info_.get_last_offset() < off_set) 
        {
          meta_info_it++;
          continue;
        }
        const vector<FragMeta>& v_in_frag_info = meta_info_it->frag_info_.v_frag_meta_;
        FragMeta fragmeta_for_search;
        fragmeta_for_search.offset_ = offset;
        vector<FragMeta>::const_iterator it =
          lower_bound(v_in_frag_info.begin(), v_in_frag_info.end(), fragmeta_for_search);
        if (it == v_in_frag_info.end())
        {
          TBSYS_LOG(ERROR, "error in frag_info");
          break;
        }
        if (it->offset_ > offset)
        {
          if (it == v_in_frag_info.begin())
          {
            TBSYS_LOG(ERROR, "error in frag_info");
            break;
          }
          else
          {
            it--;
          }
        }
        cluster_id = meta_info_it->cluster_id_;
        still_have = true;
        for(int s = 0; it != v_in_frag_info.end() && s < MAX_OUT_FRAG_INFO; it++, s++)
        {
          v_out_frag_info.push_back(*it);
          if (it->offset_ + it->size_ > offset)
          {
            still_have = false;
            break;
          }
        }
        break;
      }
      return TFS_SUCCESS;
    }

    int MetaServerService::write_frag_info(int32_t cluster_id, vector<MetaInfo>& v_meta_info,
        const vector<FragMeta>& v_frag_info)
    {
      int32_t frag_info_count = v_frag_info.size(), end_index = 0;
      int ret = TFS_SUCCESS;

      for (vector<MetaInfo>::iterator it = v_meta_info.begin();
          it != v_meta_info.end() && end_index < frag_info_count; ++it)
      {
        if (cluster_id != it->frag_info_.cluster_id_)
        {
          TBSYS_LOG(ERROR, "cluster id conflict %d <> %d", cluster_id, it->frag_info_.cluster_id_);
          ret = TFS_ERROR;
          break;
        }

        if ((ret = merge_frag_info(*it, v_frag_info, end_index))
            != TFS_SUCCESS)
        {
          TBSYS_LOG(ERROR, "merge frag info fail. ret: %d", ret); // too many fraginfos in one record
          break;
        }
      }

      if (TFS_SUCCESS == ret && end_index != frag_info_count) // remain fraginfo to insert
      {
        if ((ret = insert_frag_info(v_meta_info, v_frag_info, end_index)) != TFS_SUCCESS)
        {
          TBSYS_LOG(ERROR, "insert frag info fail. ret: %d", ret); // too many metainfos record in one file
        }
      }

      return ret;
    }

    int MetaServerService::update_write_frag_info(vector<MetaInfo>& v_meta_info, const int64_t app_id,
        const int64_t uid, MetaInfo& p_meta_info,
        const char* pname, const int32_t pname_len,
        char* name, const int32_t name_len)
    {
      int ret = TFS_SUCCESS;
      int64_t pos = 0;
      int32_t tmp_name_len = name_len;
      for (vector<MetaInfo>::iterator it = v_meta_info.begin();
          it != v_meta_info.end();
          ++it)
      {

        if (it != v_meta_info.begin())
        {
          pos = name_len;
          tmp_name_len = name_len + sizeof(int64_t);
          common::Serialization::set_int64(name, MAX_FILE_PATH_LEN, pos, it->frag_info_.v_frag_meta_[0].offset_);
        }
        if ((ret = store_manager_->insert(app_id, uid, p_meta_info.pid_, pname, pname_len,
                p_meta_info.id_, name, tmp_name_len, PWRITE_FILE))
            != TFS_SUCCESS)
        {
          TBSYS_LOG(ERROR, "insert new meta info fail. ret: %d", ret);
          break;
        }
      }
      return ret;
    }

    int MetaServerService::insert_frag_info(vector<MetaInfo>& v_meta_info, const vector<FragMeta>& src_v_frag_info,
        int32_t& end_index)
    {
      int ret = TFS_SUCCESS;
      if (end_index >= static_cast<int32_t>(src_v_frag_info.size()))
      {
        ret = TFS_ERROR;
        TBSYS_LOG(ERROR, "overflow index. %d >= %d", end_index, src_v_frag_info.size());
      }
      else
      {
        int32_t left_frag_info_count = src_v_frag_info.size() - end_index;
        vector<FragMeta>::const_iterator start_it = src_v_frag_info.begin();
        // TODO: iterator instead
        advance(start_it, end_index);
        vector<FragMeta>::const_iterator end_it = start_it;

        while (left_frag_info_count > 0)
        {
          if (left_frag_info_count <= SOFT_MAX_FRAG_INFO_COUNT)
          {
            advance(end_it, left_frag_info_count);
            left_frag_info_count = 0;
          }
          else
          {
            advance(end_it, SOFT_MAX_FRAG_INFO_COUNT);
            left_frag_info_count -= SOFT_MAX_FRAG_INFO_COUNT;
          }

          v_meta_info.push_back(MetaInfo(FragInfo(vector<FragMeta>(start_it, end_it))));
          start_it = end_it;
        }
      }
      return ret;
    }

    int MetaServerService::merge_frag_info(const MetaInfo& meta_info, const vector<FragMeta>& src_v_frag_info,
        int32_t& end_index)
    {
      int ret = TFS_SUCCESS;
      if (end_index >= static_cast<int32_t>(src_v_frag_info.size()))
      {
        ret = TFS_ERROR;
        TBSYS_LOG(WARN, "overflow index. %d >= %d", end_index, src_v_frag_info.size());
      }
      else
      {
        vector<FragMeta>::const_iterator src_it = src_v_frag_info.begin();
        vector<FragMeta>& dest_v_frag_info = const_cast<vector<FragMeta>& >(meta_info.frag_info_.v_frag_meta_);
        vector<FragMeta>::iterator dest_it = dest_v_frag_info.begin();
        int32_t frag_info_count = dest_v_frag_info.size();
        // TODO: no need, iterator instead ... 
        advance(src_it, end_index);

        if (src_it != src_v_frag_info.end() && ((meta_info.get_offset() + meta_info.size_) <= src_it->offset_)) // append
        {
          fast_merge_frag_info(dest_v_frag_info, src_v_frag_info, src_it, end_index);
        }
        // just merge in orignal container
        for (; src_it != src_v_frag_info.end() && dest_it != dest_v_frag_info.end() &&
            frag_info_count <= HARD_MAX_FRAG_INFO_COUNT;
            ++frag_info_count)
        {
          if (src_it->offset_ < dest_it->offset_)
          {
            if (src_it->offset_ + src_it->size_ <= dest_it->offset_) // fill hole
            {
              // expensive op... 
              dest_it = dest_v_frag_info.insert(dest_it, *src_it);
              src_it++;
              end_index++;
            }
            else              // overlap
            {
              TBSYS_LOG(ERROR, "update overlap error. ");
              ret = UPDATE_FRAG_INFO_ERROR;
              break;
            }
          }
          else if (src_it->offset_ < dest_it->offset_ + dest_it->size_) // overlap
          {
            TBSYS_LOG(ERROR, "update overlap error. ");
            ret = UPDATE_FRAG_INFO_ERROR;
            break;
          }
          else
          {
            dest_it++;
          }
        }

        if (ret == TFS_SUCCESS && dest_it == dest_v_frag_info.end()) // fill hole over, still can append
        {
          fast_merge_frag_info(dest_v_frag_info, src_v_frag_info, src_it, end_index);
        }
      }
      return ret;
    }

    // just append to dest from src until match threshold
    int MetaServerService::fast_merge_frag_info(vector<FragMeta>& dest_v_frag_info,
        const vector<FragMeta>& src_v_frag_info,
        vector<FragMeta>::const_iterator& src_it,
        int32_t& end_index)
    {
      int32_t frag_info_count = dest_v_frag_info.size();
      for (; src_it != src_v_frag_info.end() && frag_info_count < SOFT_MAX_FRAG_INFO_COUNT;
          src_it++, end_index++, frag_info_count++)
      {
        dest_v_frag_info.push_back(*src_it);
      }
      return TFS_SUCCESS;
    }

    // find first one whose offset is not large than offset
    vector<FragMeta>::const_iterator
      MetaServerService::lower_find(const vector<FragMeta>& v_frag_info, const int64_t offset)
      {
        vector<FragMeta>::const_iterator it = 
          lower_bound(const_cast<vector<FragMeta>&>(v_frag_info).begin(),
              const_cast<vector<FragMeta>&>(v_frag_info).end(), offset, FragMetaComp());

        if (it->offset_ != offset && it != v_frag_info.begin())
        {
          vector<FragMeta>::const_iterator pre_it = it;
          --pre_it;
          if (pre_it->offset_ + pre_it->size_ > offset)
          {
            --it;
          }
        }

        return it;
      }

    // find first one whose offset is not less than offset
    vector<FragMeta>::const_iterator
      MetaServerService::upper_find(const vector<FragMeta>& v_frag_info, const int64_t offset)
      {
        return lower_bound(const_cast<vector<FragMeta>&>(v_frag_info).begin(),
            const_cast<vector<FragMeta>&>(v_frag_info).end(), offset, FragMetaComp());
      }

    int MetaServerService::parse_name(const char* file_path, std::vector<std::string>& v_name)
    {
      int ret = TFS_ERROR;
      if ((file_path[0] != '/') || (strlen(file_path) > (MAX_FILE_PATH_LEN -1)))
      {
        TBSYS_LOG(WARN, "file_path(%s) is invalid, length(%d)", file_path, strlen(file_path));
      }
      else
      {
        v_name.clear();
        v_name.push_back("/");
        Func::split_string(file_path, '/', v_name);
        ret = TFS_SUCCESS;
      }
      return ret;
    }

    int32_t MetaServerService::get_depth(const std::vector<std::string>& v_name)
    {
      return static_cast<int32_t>(v_name.size() - 1);
    }

    // add length to file name, fit to data struct
    int MetaServerService::get_name(const std::vector<std::string>& v_name, const int32_t depth, char* buffer, const int32_t buffer_len, int32_t& name_len)
    {
      name_len = 0;

      int ret = TFS_SUCCESS;
      if (depth < 0 || depth >= static_cast<int32_t>(v_name.size()))
      {
        TBSYS_LOG(ERROR, "depth is less than 1 or more than depth, %d", depth);
        ret = TFS_ERROR;
      }
      else
      {
        name_len = strlen(v_name[depth].c_str()) + 1;
        if (name_len >= buffer_len || buffer == NULL)
        {
          TBSYS_LOG(ERROR, "buffer is not enough or buffer is null, %d < %d", buffer_len, name_len);
          ret = TFS_ERROR;
        }
        else
        {
          snprintf(buffer, name_len + 1, "%c%s", char(name_len - 1), v_name[depth].c_str());
          TBSYS_LOG(DEBUG, "old_name: %s, new_name: %s, name_len: %d", v_name[depth].c_str(), buffer, name_len);
        }
      }

      return ret;
    }

    int MetaServerService::get_p_meta_info(const int64_t app_id, const int64_t uid, const std::vector<std::string> & v_name, MetaInfo& out_meta_info, int32_t& name_len)
    {
      int ret = TFS_ERROR;
      int32_t depth = get_depth(v_name);

      char name[MAX_FILE_PATH_LEN];
      std::vector<MetaInfo> tmp_v_meta_info;
      int64_t pid = 0;
      for (int32_t i = 0; i < depth; i++)
      {
        if ((ret = get_name(v_name, i, name, MAX_FILE_PATH_LEN, name_len)) != TFS_SUCCESS)
        {
          break;
        }
        ret = store_manager_->select(app_id, uid, pid, name, name_len, tmp_v_meta_info);

        if (ret != TFS_SUCCESS)
        {
          TBSYS_LOG(ERROR, "select name(%s) failed, ret: %d", name, ret);
          break;
        }
        std::vector<MetaInfo>::const_iterator iner_iter;
        for (iner_iter = tmp_v_meta_info.begin(); iner_iter != tmp_v_meta_info.end(); iner_iter++)
        {
          TBSYS_LOG(DEBUG, "***%s*** -> ***%s***, ret: %d", iner_iter->name_.c_str(), name, memcmp(iner_iter->name_.c_str(), name, name_len));
          if (memcmp(iner_iter->name_.c_str(), name, name_len) == 0 && (pid == iner_iter->pid_))
          {
            pid = iner_iter->id_;
            // get parent info
            if (i == depth - 1)
            {
              out_meta_info = (*iner_iter);
              ret = TFS_SUCCESS;
            }
            break;
          }
        }
        if (iner_iter == tmp_v_meta_info.end())
        {
          ret = TFS_ERROR;
          TBSYS_LOG(DEBUG, "file(%s) not found, ret: %d", name, ret);
          break;
        }
      }
      return ret;
    }

    int MetaServerService::create_top_dir(const int64_t app_id, const int64_t uid)
    {
      char name[3];
      name[0] = 1;
      name[1] = '/';
      name[2] = '\0';

      return store_manager_->insert(app_id, uid, 0, "", 0, 0, name, 2, DIRECTORY);
    }
  }
}
                                                                                                                                                                                                                                                                                                                                      meta_store_manager.cpp                                                                              0000644 0141723 0001014 00000013522 11613721322 014343  0                                                                                                    ustar   daoan                           daoan                                                                                                                                                                                                                  /*
* (C) 2007-2010 Alibaba Group Holding Limited.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
*
* Version: $Id
*
* Authors:
*   chuyu <chuyu@taobao.com>
*      - initial release
*
*/
#include "meta_store_manager.h"

using namespace tfs::common;
namespace tfs
{
  namespace namemetaserver
  {
    MetaStoreManager::MetaStoreManager()
    {
      database_helper_ = new MysqlDatabaseHelper();  
      database_helper_->set_conn_param(ConnStr::mysql_conn_str_.c_str(), ConnStr::mysql_user_.c_str(), ConnStr::mysql_password_.c_str());
      //meta_cache_handler_ = new MetaCacheHandler();

      database_helper_->connect();
    }

    MetaStoreManager::~MetaStoreManager()
    {
      tbsys::gDelete(database_helper_);
      //tbsys::gDelete(meta_cache_handler_);
    }


    int MetaStoreManager::select(const int64_t app_id, const int64_t uid, const int64_t pid, const char* name, const int32_t name_len, std::vector<MetaInfo>& out_v_meta_info)
    {
      int ret = TFS_ERROR;
      ret = database_helper_->ls_meta_info(out_v_meta_info, app_id, uid, pid, name, name_len);
      return ret;
    }

    int MetaStoreManager::insert(const int64_t app_id, const int64_t uid,
        const int64_t ppid, const char* pname, const int32_t pname_len, 
        const int64_t pid, const char* name, const int32_t name_len, const FileType type, MetaInfo* meta_info)
    {
      int ret = TFS_ERROR;
      int status = TFS_ERROR;
      int64_t proc_ret = 0;
      
      if (type == NORMAL_FILE)
      {
        status = database_helper_->create_file(app_id, uid, ppid, pid, pname, pname_len, name, name_len, proc_ret);
        if (TFS_SUCCESS != status)
        {
          TBSYS_LOG(DEBUG, "database helper create file, status: %d", status);
        }
      }
      else if (type == DIRECTORY)
      {
        int64_t id = 0;
        status = database_helper_->get_nex_val(id);
        if (TFS_SUCCESS == status && id != 0)
        {
          status = database_helper_->create_dir(app_id, uid, ppid, pname, pname_len, pid, id, name, name_len, proc_ret);
          if (TFS_SUCCESS != status)
          {
            TBSYS_LOG(DEBUG, "database helper create dir, status: %d", status);
          }
        }
      }
      else if (type == PWRITE_FILE)
      {
        int64_t frag_len = meta_info->get_frag_length();
        if (frag_len > 65535)
        {
          TBSYS_LOG(ERROR, "meta info is too long(%d > %d)", frag_len, 65535);
          ret = TFS_ERROR;
        }
        else
        {
          int64_t pos = 0;
          char* frag_info = (char*) malloc(frag_len); 
          status = meta_info->get_frag_info(frag_info, frag_len, pos);
          if (TFS_SUCCESS != status)
          {
            TBSYS_LOG(ERROR, "get meta info failed, status: %d ", status);
          }
          else
          {
            status = database_helper_->pwrite_file(app_id, uid, pid, name, name_len, meta_info->size_, meta_info->ver_no_, frag_info, frag_len, proc_ret);
            if (TFS_SUCCESS != status)
            {
              TBSYS_LOG(DEBUG, "database helper pwrite file, status: %d", status);
            }
          }
        }
      }

      if ((static_cast<int32_t>(proc_ret)) > 0)
      {
        ret = TFS_SUCCESS;
      }
      return ret;
    }

    int MetaStoreManager::update(const int64_t app_id, const int64_t uid, 
        const int64_t s_ppid, const int64_t s_pid, const char* s_pname, const int32_t s_pname_len,
        const int64_t d_ppid, const int64_t d_pid, const char* d_pname, const int32_t d_pname_len,
        const char* s_name, const int32_t s_name_len,
        const char* d_name, const int32_t d_name_len,
        const FileType type)
    {
      int ret = TFS_ERROR;
      int status = TFS_ERROR;
      int64_t proc_ret = 0;
      
      if (type & NORMAL_FILE)
      {
        status = database_helper_->mv_file(app_id, uid, s_ppid, s_pid, s_pname, s_pname_len,
        d_ppid, d_pid, d_pname, d_pname_len, s_name, s_name_len, d_name, d_name_len, proc_ret);
        if (TFS_SUCCESS != status)
        {
          TBSYS_LOG(DEBUG, "database helper mv file, status: %d", status);
        }
      }
      else if (type & DIRECTORY)
      {
        status = database_helper_->mv_dir(app_id, uid, s_ppid, s_pid, s_pname, s_pname_len,
        d_ppid, d_pid, d_pname, d_pname_len, s_name, s_name_len, d_name, d_name_len, proc_ret);
        if (TFS_SUCCESS != status)
        {
          TBSYS_LOG(DEBUG, "database helper mv dir, status: %d", status);
        }
      }

      if ((static_cast<int32_t>(proc_ret)) > 0)
      {
        ret = TFS_SUCCESS;
      }
      return ret;
    }

    int MetaStoreManager::remove(const int64_t app_id, const int64_t uid, const int64_t ppid,
            const char* pname, const int64_t pname_len, const int64_t pid, const int64_t id,
            const char* name, const int64_t name_len,
            const FileType type)
    {
      int ret = TFS_ERROR;
      int status = TFS_ERROR;
      int64_t proc_ret = 0;
      
      if (type & NORMAL_FILE)
      {
        status = database_helper_->rm_file(app_id, uid, ppid, pid, pname, pname_len, name, name_len, proc_ret);
        if (TFS_SUCCESS != status)
        {
          TBSYS_LOG(DEBUG, "database helper rm file, status: %d", status);
        }
      }
      else if (type & DIRECTORY)
      {
        if (id == 0)
        {
          TBSYS_LOG(DEBUG, "wrong type, target is file.");
        }
        else
        {
          status = database_helper_->rm_dir(app_id, uid, ppid, pname, pname_len, pid, id, name, name_len, proc_ret);
          if (TFS_SUCCESS != status)
          {
            TBSYS_LOG(DEBUG, "database helper rm dir, status: %d", status);
          }
        }
      }

      if ((static_cast<int32_t>(proc_ret)) > 0)
      {
        ret = TFS_SUCCESS;
      }
      return ret;
    }
  }
}
                                                                                                                                                                              mysql_database_helper.cpp                                                                           0000644 0141723 0001014 00000105236 11613721323 015044  0                                                                                                    ustar   daoan                           daoan                                                                                                                                                                                                                  /*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id
 *
 * Authors:
 *   daoan <daoan@taobao.com>
 *      - initial release
 *
 */
#include "mysql_database_helper.h"

#include <mysql/errmsg.h>
#include <vector>
#include "common/define.h"

using namespace std;
namespace
{

  static int split_string(const char* line, const char del, vector<string> & fields)
  {
    const char* start = line;
    const char* p = NULL;
    char buffer[256];
    while (start != NULL)
    {
      p = strchr(start, del);
      if (p != NULL)
      {
        memset(buffer, 0, 256);
        strncpy(buffer, start, p - start);
        if (strlen(buffer) > 0) fields.push_back(buffer);
        start = p + 1;
      }
      else
      {
        memset(buffer, 0, 256);
        strcpy(buffer, start);
        if (strlen(buffer) > 0) fields.push_back(buffer);
        break;
      }
    }
    return fields.size();
  }
}

namespace tfs
{
  namespace namemetaserver
  {
    using namespace common;
    MysqlDatabaseHelper::MysqlDatabaseHelper()
    {
      stmt_ = NULL;
    }
    MysqlDatabaseHelper::~MysqlDatabaseHelper()
    {
      close();
      mysql_server_end();
    }
    int MysqlDatabaseHelper::connect()
    {
      int ret = TFS_SUCCESS;
      if (is_connected_)
      {
        close();
      }

      if (!init_mysql(conn_str_, user_name_, passwd_))
      {
        ret = TFS_ERROR;
      }
      else if (!open_mysql())
      {
        ret = TFS_ERROR;
      }
      if (TFS_SUCCESS == ret)
      {
        char sql[1024];
        snprintf(sql, 1024, "select pid, name, id, UNIX_TIMESTAMP(create_time), "
            "UNIX_TIMESTAMP(modify_time), size, ver_no, meta_info from t_meta_info "
            "where app_id = ? and uid = ? and pid >= ? and name >= ? limit %d", ROW_LIMIT);
        if (NULL != stmt_)
        {
          mysql_stmt_free_result(stmt_);
          mysql_stmt_close(stmt_);
        }
        stmt_ = mysql_stmt_init(&mysql_.mysql);
        int status = mysql_stmt_prepare(stmt_, sql, strlen(sql));
        if (status)
        {
          TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
              mysql_stmt_error(stmt_), mysql_stmt_errno(stmt_));
          ret = TFS_ERROR;
        }
        if (TFS_SUCCESS == ret)
        {
          memset(ps_params_, 0, sizeof (ps_params_));
          ps_params_[0].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params_[0].buffer = (char *) &app_id_;
          ps_params_[0].length = 0;
          ps_params_[0].is_null = 0;

          ps_params_[1].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params_[1].buffer = (char *) &uid_;
          ps_params_[1].length = 0;
          ps_params_[1].is_null = 0;

          ps_params_[2].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params_[2].buffer = (char *) &pid_;
          ps_params_[2].length = 0;
          ps_params_[2].is_null = 0;

          ps_params_[3].buffer_type = MYSQL_TYPE_VAR_STRING;
          ps_params_[3].buffer = (char *) pname_;
          ps_params_[3].length = &pname_len_;
          ps_params_[3].is_null = 0;

          status = mysql_stmt_bind_param(stmt_, ps_params_);
          if (status)
          {
            TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
                mysql_stmt_error(stmt_), mysql_stmt_errno(stmt_));
            ret = TFS_ERROR;
          }
        }
      }

      is_connected_ = TFS_SUCCESS == ret;
      return ret;
    }

    int MysqlDatabaseHelper::close()
    {
      if (NULL != stmt_)
      {
        mysql_stmt_free_result(stmt_);
        mysql_stmt_close(stmt_);
        stmt_ = NULL;
      }
      close_mysql();
      is_connected_ = false;
      return TFS_SUCCESS;
    }
    int MysqlDatabaseHelper::ls_meta_info(std::vector<MetaInfo>& out_v_meta_info,
        const int64_t app_id, const int64_t uid,
            const int64_t pid, const char* name, const int32_t name_len)
    {
      int ret = TFS_ERROR;
      out_v_meta_info.clear();
      tbutil::Mutex::Lock lock(mutex_);
      if (!is_connected_)
      {
        connect();
      }
      if (is_connected_)
      {
        ret = TFS_SUCCESS;
        int status;
        app_id_ = app_id;
        uid_ = uid;
        pid_ = pid;
        if (NULL != name && name_len > 0 && name_len < META_NAME_LEN)
        {
          memcpy(pname_, name, name_len);
          pname_len_ = name_len;
        }
        else
        {
          pname_[0] = 0;
          pname_len_ = 1;
        }
        status = mysql_stmt_execute(stmt_);
        if (status)
        {
          TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
              mysql_stmt_error(stmt_), mysql_stmt_errno(stmt_));
          ret = TFS_ERROR;
        }
        if (TFS_SUCCESS == ret)
        {
          MYSQL_BIND rs_bind[8];  /* for output buffers */
          my_bool    is_null[8];
          int64_t o_pid = 0;
          char o_name[META_NAME_LEN];
          unsigned long o_name_len = 0;
          int64_t o_id = 0;
          int32_t o_create_time = 0;
          int32_t o_modify_time = 0;
          int64_t o_size = 0;
          int16_t o_ver_no = 0;
          static char o_slide_info[SLIDE_INFO_LEN];
          unsigned long o_slide_info_len = 0;

          memset(rs_bind, 0, sizeof (rs_bind) );

          /* set up and bind result set output buffers */
          rs_bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
          rs_bind[0].is_null = &is_null[0];
          rs_bind[0].buffer = (char *) &o_pid;
          rs_bind[0].is_unsigned = 1;

          rs_bind[1].buffer_type = MYSQL_TYPE_STRING;
          rs_bind[1].is_null = &is_null[1];
          rs_bind[1].buffer = (char *) o_name;
          rs_bind[1].buffer_length = META_NAME_LEN;
          rs_bind[1].length= &o_name_len;

          rs_bind[2].buffer_type = MYSQL_TYPE_LONGLONG;
          rs_bind[2].is_null = &is_null[2];
          rs_bind[2].buffer = (char *) &o_id;

          rs_bind[3].buffer_type = MYSQL_TYPE_LONG;
          rs_bind[3].is_null = &is_null[3];
          rs_bind[3].buffer = (char *) &o_create_time;

          rs_bind[4].buffer_type = MYSQL_TYPE_LONG;
          rs_bind[4].is_null = &is_null[4];
          rs_bind[4].buffer = (char *) &o_modify_time;

          rs_bind[5].buffer_type = MYSQL_TYPE_LONGLONG;
          rs_bind[5].is_null = &is_null[5];
          rs_bind[5].buffer = (char *) &o_size;

          rs_bind[6].buffer_type = MYSQL_TYPE_SHORT;
          rs_bind[6].is_null = &is_null[6];
          rs_bind[6].buffer = (char *) &o_ver_no;

          rs_bind[7].buffer_type = MYSQL_TYPE_BLOB;
          rs_bind[7].is_null = &is_null[7];
          rs_bind[7].buffer_length = SLIDE_INFO_LEN;
          rs_bind[7].buffer = (char *) o_slide_info;
          rs_bind[7].length= &o_slide_info_len;

          status = mysql_stmt_bind_result(stmt_, rs_bind);
          if (status)
          {
            TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
                mysql_stmt_error(stmt_), mysql_stmt_errno(stmt_));
            ret = TFS_ERROR;
          }
          if (TFS_SUCCESS == ret)
          {
            status = mysql_stmt_store_result(stmt_);
            if (status)
            {
              TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt_), mysql_stmt_errno(stmt_));
              ret = TFS_ERROR;
            }
          }
          while (TFS_SUCCESS == ret )
          {
            status = mysql_stmt_fetch(stmt_);
            if (1 == status)
            {
              TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt_), mysql_stmt_errno(stmt_));
              ret = TFS_ERROR;
              break;
            }else if (MYSQL_NO_DATA == status)
            {
              break;
            }
            else if (MYSQL_DATA_TRUNCATED == status)
            {
              TBSYS_LOG(ERROR, "MYSQL_DATA_TRUNCATED");
              break;
            }
            MetaInfo tmp;
            tmp.name_.assign(o_name, o_name_len);
            tmp.pid_ = o_pid;
            tmp.id_ = o_id;
            tmp.create_time_ = o_create_time;
            tmp.modify_time_ = o_modify_time;
            tmp.size_ = o_size;
            tmp.ver_no_ = o_ver_no;
            //TODO slide_info_;
            out_v_meta_info.push_back(tmp);
          }
          mysql_next_result(&mysql_.mysql); //mysql bugs, we must have this
        }
      }
      if (TFS_SUCCESS != ret)
      {
        close();
      }
      return ret;
    }

    int MysqlDatabaseHelper::create_dir(const int64_t app_id, const int64_t uid,
        const int64_t ppid, const char* pname, const int32_t pname_len,
        const int64_t pid, const int64_t id, const char* name, const int32_t name_len,
        int64_t& mysql_proc_ret)
    {
      int ret = TFS_ERROR;
      MYSQL_STMT *stmt;
      MYSQL_BIND ps_params[7];  /* input parameter buffers */
      unsigned long _pname_len = pname_len;
      unsigned long _name_len = name_len;
      int        status;
      const char* str = "CALL create_dir(?, ?, ?, ?, ?, ?, ?)";

      mysql_proc_ret = 0;

      tbutil::Mutex::Lock lock(mutex_);
      if (!is_connected_)
      {
        connect();
      }
      if (is_connected_)
      {
        stmt = mysql_stmt_init(&mysql_.mysql);
        ret = TFS_SUCCESS;
        status = mysql_stmt_prepare(stmt, str, strlen(str)); //TODO prepare once
        if (status)
        {
          TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
              mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
          ret = TFS_ERROR;
        }
        if (TFS_SUCCESS == ret)
        {
          memset(ps_params, 0, sizeof (ps_params));
          ps_params[0].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[0].buffer = (char *) &app_id;
          ps_params[0].length = 0;
          ps_params[0].is_null = 0;

          ps_params[1].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[1].buffer = (char *) &uid;
          ps_params[1].length = 0;
          ps_params[1].is_null = 0;

          ps_params[2].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[2].buffer = (char *) &ppid;
          ps_params[2].length = 0;
          ps_params[2].is_null = 0;

          ps_params[3].buffer_type = MYSQL_TYPE_VAR_STRING;
          ps_params[3].buffer = (char *) pname;
          ps_params[3].length = &_pname_len;
          ps_params[3].is_null = 0;

          ps_params[4].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[4].buffer = (char *) &pid;
          ps_params[4].length = 0;
          ps_params[4].is_null = 0;

          ps_params[5].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[5].buffer = (char *) &id;
          ps_params[5].length = 0;
          ps_params[5].is_null = 0;

          ps_params[6].buffer_type = MYSQL_TYPE_VAR_STRING;
          ps_params[6].buffer = (char *) name;
          ps_params[6].length = &_name_len;
          ps_params[6].is_null = 0;

          //ps_params[7].buffer_type = MYSQL_TYPE_LONG;
          //ps_params[7].buffer = (char *) &mysql_proc_ret;
          //ps_params[7].length = 0;
          //ps_params[7].is_null = 0;

          status = mysql_stmt_bind_param(stmt, ps_params);
          if (status)
          {
            TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
                mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
            ret = TFS_ERROR;
          }
          if (TFS_SUCCESS == ret)
          {
            if (!excute_stmt(stmt, mysql_proc_ret))
            {
              ret = TFS_ERROR;
            }
          }

        }
      }
      if (TFS_SUCCESS != ret)
      {
        close();
      }
      return ret;
    }
    int MysqlDatabaseHelper::rm_dir(const int64_t app_id, const int64_t uid, const int64_t ppid,
            const char* pname, const int32_t pname_len, const int64_t pid, const int64_t id,
            const char* name, const int32_t name_len, int64_t& mysql_proc_ret)
    {
      int ret = TFS_ERROR;
      MYSQL_STMT *stmt;
      MYSQL_BIND ps_params[7];  /* input parameter buffers */
      unsigned long _pname_len = pname_len;
      unsigned long _name_len = name_len;
      int        status;
      const char* str = "CALL rm_dir(?, ?, ?, ?, ?, ?, ?)";

      mysql_proc_ret = 0;

      tbutil::Mutex::Lock lock(mutex_);
      if (!is_connected_)
      {
        connect();
      }
      if (is_connected_)
      {
        stmt = mysql_stmt_init(&mysql_.mysql);
        ret = TFS_SUCCESS;
        status = mysql_stmt_prepare(stmt, str, strlen(str)); //TODO prepare once
        if (status)
        {
          TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
              mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
          ret = TFS_ERROR;
        }
        if (TFS_SUCCESS == ret)
        {
          memset(ps_params, 0, sizeof (ps_params));
          ps_params[0].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[0].buffer = (char *) &app_id;
          ps_params[0].length = 0;
          ps_params[0].is_null = 0;

          ps_params[1].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[1].buffer = (char *) &uid;
          ps_params[1].length = 0;
          ps_params[1].is_null = 0;

          ps_params[2].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[2].buffer = (char *) &ppid;
          ps_params[2].length = 0;
          ps_params[2].is_null = 0;

          ps_params[3].buffer_type = MYSQL_TYPE_VAR_STRING;
          ps_params[3].buffer = (char *) pname;
          ps_params[3].length = &_pname_len;
          ps_params[3].is_null = 0;

          ps_params[4].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[4].buffer = (char *) &pid;
          ps_params[4].length = 0;
          ps_params[4].is_null = 0;

          ps_params[5].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[5].buffer = (char *) &id;
          ps_params[5].length = 0;
          ps_params[5].is_null = 0;

          ps_params[6].buffer_type = MYSQL_TYPE_VAR_STRING;
          ps_params[6].buffer = (char *) name;
          ps_params[6].length = &_name_len;
          ps_params[6].is_null = 0;

          status = mysql_stmt_bind_param(stmt, ps_params);
          if (status)
          {
            TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
                mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
            ret = TFS_ERROR;
          }
          if (TFS_SUCCESS == ret)
          {
            if (!excute_stmt(stmt, mysql_proc_ret))
            {
              ret = TFS_ERROR;
            }
          }

        }
      }
      if (TFS_SUCCESS != ret)
      {
        close();
      }
      return ret;
    }

    int MysqlDatabaseHelper::mv_dir(const int64_t app_id, const int64_t uid,
        const int64_t s_ppid, const int64_t s_pid, const char* s_pname, const int32_t s_pname_len,
        const int64_t d_ppid, const int64_t d_pid, const char* d_pname, const int32_t d_pname_len,
        const char* s_name, const int32_t s_name_len,
        const char* d_name, const int32_t d_name_len,
        int64_t& mysql_proc_ret)
    {
      int ret = TFS_ERROR;
      MYSQL_STMT *stmt;
      MYSQL_BIND ps_params[10];  /* input parameter buffers */
      unsigned long _s_pname_len = s_pname_len;
      unsigned long _d_pname_len = d_pname_len;
      unsigned long _s_name_len = s_name_len;
      unsigned long _d_name_len = d_name_len;
      int        status;
      const char* str = "CALL mv_dir(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

      mysql_proc_ret = 0;

      tbutil::Mutex::Lock lock(mutex_);
      if (!is_connected_)
      {
        connect();
      }
      if (is_connected_)
      {
        stmt = mysql_stmt_init(&mysql_.mysql);
        ret = TFS_SUCCESS;
        status = mysql_stmt_prepare(stmt, str, strlen(str)); //TODO prepare once
        if (status)
        {
          TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
              mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
          ret = TFS_ERROR;
        }
        if (TFS_SUCCESS == ret)
        {
          memset(ps_params, 0, sizeof (ps_params));
          ps_params[0].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[0].buffer = (char *) &app_id;
          ps_params[0].length = 0;
          ps_params[0].is_null = 0;

          ps_params[1].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[1].buffer = (char *) &uid;
          ps_params[1].length = 0;
          ps_params[1].is_null = 0;

          ps_params[2].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[2].buffer = (char *) &s_ppid;
          ps_params[2].length = 0;
          ps_params[2].is_null = 0;

          ps_params[3].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[3].buffer = (char *) &s_pid;
          ps_params[3].length = 0;
          ps_params[3].is_null = 0;

          ps_params[4].buffer_type = MYSQL_TYPE_VAR_STRING;
          ps_params[4].buffer = (char *) s_pname;
          ps_params[4].length = &_s_pname_len;
          ps_params[4].is_null = 0;

          ps_params[5].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[5].buffer = (char *) &d_ppid;
          ps_params[5].length = 0;
          ps_params[5].is_null = 0;

          ps_params[6].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[6].buffer = (char *) &d_pid;
          ps_params[6].length = 0;
          ps_params[6].is_null = 0;

          ps_params[7].buffer_type = MYSQL_TYPE_VAR_STRING;
          ps_params[7].buffer = (char *) d_pname;
          ps_params[7].length = &_d_pname_len;
          ps_params[7].is_null = 0;

          ps_params[8].buffer_type = MYSQL_TYPE_VAR_STRING;
          ps_params[8].buffer = (char *) s_name;
          ps_params[8].length = &_s_name_len;
          ps_params[8].is_null = 0;

          ps_params[9].buffer_type = MYSQL_TYPE_VAR_STRING;
          ps_params[9].buffer = (char *) d_name;
          ps_params[9].length = &_d_name_len;
          ps_params[9].is_null = 0;


          status = mysql_stmt_bind_param(stmt, ps_params);
          if (status)
          {
            TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
                mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
            ret = TFS_ERROR;
          }
          if (TFS_SUCCESS == ret)
          {
            if (!excute_stmt(stmt, mysql_proc_ret))
            {
              ret = TFS_ERROR;
            }
          }

        }
      }
      if (TFS_SUCCESS != ret)
      {
        close();
      }
      return ret;
    }
    int MysqlDatabaseHelper::create_file(const int64_t app_id, const int64_t uid,
            const int64_t ppid, const int64_t pid, const char* pname, const int32_t pname_len,
            const char* name, const int32_t name_len, int64_t& mysql_proc_ret)
    {
      int ret = TFS_ERROR;
      MYSQL_STMT *stmt;
      MYSQL_BIND ps_params[6];  /* input parameter buffers */
      unsigned long _pname_len = pname_len;
      unsigned long _name_len = name_len;
      int        status;
      const char* str = "CALL create_file(?, ?, ?, ?, ?, ?)";

      mysql_proc_ret = 0;

      tbutil::Mutex::Lock lock(mutex_);
      if (!is_connected_)
      {
        connect();
      }
      if (is_connected_)
      {
        stmt = mysql_stmt_init(&mysql_.mysql);
        ret = TFS_SUCCESS;
        status = mysql_stmt_prepare(stmt, str, strlen(str)); //TODO prepare once
        if (status)
        {
          TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
              mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
          ret = TFS_ERROR;
        }
        if (TFS_SUCCESS == ret)
        {
          memset(ps_params, 0, sizeof (ps_params));
          ps_params[0].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[0].buffer = (char *) &app_id;
          ps_params[0].length = 0;
          ps_params[0].is_null = 0;

          ps_params[1].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[1].buffer = (char *) &uid;
          ps_params[1].length = 0;
          ps_params[1].is_null = 0;

          ps_params[2].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[2].buffer = (char *) &ppid;
          ps_params[2].length = 0;
          ps_params[2].is_null = 0;

          ps_params[3].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[3].buffer = (char *) &pid;
          ps_params[3].length = 0;
          ps_params[3].is_null = 0;

          ps_params[4].buffer_type = MYSQL_TYPE_VAR_STRING;
          ps_params[4].buffer = (char *) pname;
          ps_params[4].length = &_pname_len;
          ps_params[4].is_null = 0;

          ps_params[5].buffer_type = MYSQL_TYPE_VAR_STRING;
          ps_params[5].buffer = (char *) name;
          ps_params[5].length = &_name_len;
          ps_params[5].is_null = 0;

          status = mysql_stmt_bind_param(stmt, ps_params);
          if (status)
          {
            TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
                mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
            ret = TFS_ERROR;
          }
          if (TFS_SUCCESS == ret)
          {
            if (!excute_stmt(stmt, mysql_proc_ret))
            {
              ret = TFS_ERROR;
            }
          }

        }
      }
      if (TFS_SUCCESS != ret)
      {
        close();
      }
      return ret;
    }
    int MysqlDatabaseHelper::rm_file(const int64_t app_id, const int64_t uid,
            const int64_t ppid, const int64_t pid, const char* pname, const int32_t pname_len,
            const char* name, const int32_t name_len, int64_t& mysql_proc_ret)
    {
      int ret = TFS_ERROR;
      MYSQL_STMT *stmt;
      MYSQL_BIND ps_params[6];  /* input parameter buffers */
      unsigned long _pname_len = pname_len;
      unsigned long _name_len = name_len;
      int        status;
      const char* str = "CALL rm_file(?, ?, ?, ?, ?, ?)";

      mysql_proc_ret = 0;

      tbutil::Mutex::Lock lock(mutex_);
      if (!is_connected_)
      {
        connect();
      }
      if (is_connected_)
      {
        stmt = mysql_stmt_init(&mysql_.mysql);
        ret = TFS_SUCCESS;
        status = mysql_stmt_prepare(stmt, str, strlen(str)); //TODO prepare once
        if (status)
        {
          TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
              mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
          ret = TFS_ERROR;
        }
        if (TFS_SUCCESS == ret)
        {
          memset(ps_params, 0, sizeof (ps_params));
          ps_params[0].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[0].buffer = (char *) &app_id;
          ps_params[0].length = 0;
          ps_params[0].is_null = 0;

          ps_params[1].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[1].buffer = (char *) &uid;
          ps_params[1].length = 0;
          ps_params[1].is_null = 0;

          ps_params[2].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[2].buffer = (char *) &ppid;
          ps_params[2].length = 0;
          ps_params[2].is_null = 0;

          ps_params[3].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[3].buffer = (char *) &pid;
          ps_params[3].length = 0;
          ps_params[3].is_null = 0;

          ps_params[4].buffer_type = MYSQL_TYPE_VAR_STRING;
          ps_params[4].buffer = (char *) pname;
          ps_params[4].length = &_pname_len;
          ps_params[4].is_null = 0;

          ps_params[5].buffer_type = MYSQL_TYPE_VAR_STRING;
          ps_params[5].buffer = (char *) name;
          ps_params[5].length = &_name_len;
          ps_params[5].is_null = 0;

          status = mysql_stmt_bind_param(stmt, ps_params);
          if (status)
          {
            TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
                mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
            ret = TFS_ERROR;
          }
          if (TFS_SUCCESS == ret)
          {
            if (!excute_stmt(stmt, mysql_proc_ret))
            {
              ret = TFS_ERROR;
            }
          }

        }
      }
      if (TFS_SUCCESS != ret)
      {
        close();
      }
      return ret;
    }
    int MysqlDatabaseHelper::pwrite_file(const int64_t app_id, const int64_t uid,
        const int64_t pid, const char* name, const int32_t name_len,
        const int64_t size, const int16_t ver_no, const char* meta_info, const int32_t meta_len,
        int64_t& mysql_proc_ret)
    {
      int ret = TFS_ERROR;
      MYSQL_STMT *stmt;
      MYSQL_BIND ps_params[7];  /* input parameter buffers */
      unsigned long _name_len = name_len;
      unsigned long _meta_len = meta_len;
      int        status;
      const char* str = "CALL pwrite_file(?, ?, ?, ?, ?, ?, ?)";

      mysql_proc_ret = 0;

      tbutil::Mutex::Lock lock(mutex_);
      if (!is_connected_)
      {
        connect();
      }
      if (is_connected_)
      {
        stmt = mysql_stmt_init(&mysql_.mysql);
        ret = TFS_SUCCESS;
        status = mysql_stmt_prepare(stmt, str, strlen(str)); //TODO prepare once
        if (status)
        {
          TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
              mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
          ret = TFS_ERROR;
        }
        if (TFS_SUCCESS == ret)
        {
          memset(ps_params, 0, sizeof (ps_params));
          ps_params[0].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[0].buffer = (char *) &app_id;
          ps_params[0].length = 0;
          ps_params[0].is_null = 0;

          ps_params[1].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[1].buffer = (char *) &uid;
          ps_params[1].length = 0;
          ps_params[1].is_null = 0;

          ps_params[2].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[2].buffer = (char *) &pid;
          ps_params[2].length = 0;
          ps_params[2].is_null = 0;

          ps_params[3].buffer_type = MYSQL_TYPE_VAR_STRING;
          ps_params[3].buffer = (char *) name;
          ps_params[3].length = &_name_len;
          ps_params[3].is_null = 0;

          ps_params[4].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[4].buffer = (char *) &size;
          ps_params[4].length = 0;
          ps_params[4].is_null = 0;

          ps_params[5].buffer_type = MYSQL_TYPE_SHORT;
          ps_params[5].buffer = (char *) &ver_no;
          ps_params[5].length = 0;
          ps_params[5].is_null = 0;

          ps_params[6].buffer_type = MYSQL_TYPE_BLOB;
          ps_params[6].buffer = (char *) meta_info;
          ps_params[6].length = &_meta_len;
          ps_params[6].is_null = 0;

          status = mysql_stmt_bind_param(stmt, ps_params);
          if (status)
          {
            TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
                mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
            ret = TFS_ERROR;
          }
          if (TFS_SUCCESS == ret)
          {
            if (!excute_stmt(stmt, mysql_proc_ret))
            {
              ret = TFS_ERROR;
            }
          }

        }
      }
      if (TFS_SUCCESS != ret)
      {
        close();
      }
      return ret;
    }
    int MysqlDatabaseHelper::mv_file(const int64_t app_id, const int64_t uid,
        const int64_t s_ppid, const int64_t s_pid, const char* s_pname, const int32_t s_pname_len,
        const int64_t d_ppid, const int64_t d_pid, const char* d_pname, const int32_t d_pname_len,
        const char* s_name, const int32_t s_name_len,
        const char* d_name, const int32_t d_name_len,
        int64_t& mysql_proc_ret)
    {
      int ret = TFS_ERROR;
      MYSQL_STMT *stmt;
      MYSQL_BIND ps_params[10];  /* input parameter buffers */
      unsigned long _s_pname_len = s_pname_len;
      unsigned long _d_pname_len = d_pname_len;
      unsigned long _s_name_len = s_name_len;
      unsigned long _d_name_len = d_name_len;
      int        status;
      const char* str = "CALL mv_file(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

      mysql_proc_ret = 0;

      tbutil::Mutex::Lock lock(mutex_);
      if (!is_connected_)
      {
        connect();
      }
      if (is_connected_)
      {
        stmt = mysql_stmt_init(&mysql_.mysql);
        ret = TFS_SUCCESS;
        status = mysql_stmt_prepare(stmt, str, strlen(str)); //TODO prepare once
        if (status)
        {
          TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
              mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
          ret = TFS_ERROR;
        }
        if (TFS_SUCCESS == ret)
        {
          memset(ps_params, 0, sizeof (ps_params));
          ps_params[0].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[0].buffer = (char *) &app_id;
          ps_params[0].length = 0;
          ps_params[0].is_null = 0;

          ps_params[1].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[1].buffer = (char *) &uid;
          ps_params[1].length = 0;
          ps_params[1].is_null = 0;

          ps_params[2].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[2].buffer = (char *) &s_ppid;
          ps_params[2].length = 0;
          ps_params[2].is_null = 0;

          ps_params[3].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[3].buffer = (char *) &s_pid;
          ps_params[3].length = 0;
          ps_params[3].is_null = 0;

          ps_params[4].buffer_type = MYSQL_TYPE_VAR_STRING;
          ps_params[4].buffer = (char *) s_pname;
          ps_params[4].length = &_s_pname_len;
          ps_params[4].is_null = 0;

          ps_params[5].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[5].buffer = (char *) &d_ppid;
          ps_params[5].length = 0;
          ps_params[5].is_null = 0;

          ps_params[6].buffer_type = MYSQL_TYPE_LONGLONG;
          ps_params[6].buffer = (char *) &d_pid;
          ps_params[6].length = 0;
          ps_params[6].is_null = 0;

          ps_params[7].buffer_type = MYSQL_TYPE_VAR_STRING;
          ps_params[7].buffer = (char *) d_pname;
          ps_params[7].length = &_d_pname_len;
          ps_params[7].is_null = 0;

          ps_params[8].buffer_type = MYSQL_TYPE_VAR_STRING;
          ps_params[8].buffer = (char *) s_name;
          ps_params[8].length = &_s_name_len;
          ps_params[8].is_null = 0;

          ps_params[9].buffer_type = MYSQL_TYPE_VAR_STRING;
          ps_params[9].buffer = (char *) d_name;
          ps_params[9].length = &_d_name_len;
          ps_params[9].is_null = 0;


          status = mysql_stmt_bind_param(stmt, ps_params);
          if (status)
          {
            TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
                mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
            ret = TFS_ERROR;
          }
          if (TFS_SUCCESS == ret)
          {
            if (!excute_stmt(stmt, mysql_proc_ret))
            {
              ret = TFS_ERROR;
            }
          }

        }
      }
      if (TFS_SUCCESS != ret)
      {
        close();
      }
      return ret;
    }
    int MysqlDatabaseHelper::get_nex_val(int64_t& next_val)
    {
      int ret = TFS_ERROR;
      MYSQL_STMT *stmt;
      int        status;
      const char* str = "CALL pid_seq_nextval()";

      next_val = 0;

      tbutil::Mutex::Lock lock(mutex_);
      if (!is_connected_)
      {
        connect();
      }
      if (is_connected_)
      {
        stmt = mysql_stmt_init(&mysql_.mysql);
        ret = TFS_SUCCESS;
        status = mysql_stmt_prepare(stmt, str, strlen(str)); //TODO prepare once
        if (status)
        {
          TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
              mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
          ret = TFS_ERROR;
        }
        if (TFS_SUCCESS == ret)
        {
            if (!excute_stmt(stmt, next_val))
            {
              ret = TFS_ERROR;
            }

        }
      }
      if (TFS_SUCCESS != ret)
      {
        close();
      }
      return ret;
    }

    bool MysqlDatabaseHelper::init_mysql(const char* mysqlconn, const char* user_name, const char* passwd)
    {
      vector<string> fields;
      split_string(mysqlconn, ':', fields);
      mysql_.isopen = false;
      if (fields.size() < 3 || NULL == user_name || NULL == passwd)
        return false;
      mysql_.host = fields[0];
      mysql_.port = atoi(fields[1].c_str());
      mysql_.user = user_name;
      mysql_.pass = passwd;
      mysql_.database = fields[2];
      mysql_.inited = true;

      int v = 5;
      mysql_init(&mysql_.mysql);
      mysql_options(&mysql_.mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const char *)&v);
      mysql_options(&mysql_.mysql, MYSQL_OPT_READ_TIMEOUT, (const char *)&v);
      mysql_options(&mysql_.mysql, MYSQL_OPT_WRITE_TIMEOUT, (const char *)&v);
      return true;
    }

    bool MysqlDatabaseHelper::open_mysql()
    {
      if (!mysql_.inited) return false;
      if (mysql_.isopen) return true;
      MYSQL *conn = mysql_real_connect(
          &mysql_.mysql,
          mysql_.host.c_str(),
          mysql_.user.c_str(),
          mysql_.pass.c_str(),
          mysql_.database.c_str(),
          mysql_.port, NULL, CLIENT_MULTI_STATEMENTS);
      if (!conn)
      {
        TBSYS_LOG(ERROR, "connect mysql database (%s:%d:%s:%s:%s) error(%s)",
            mysql_.host.c_str(), mysql_.port, mysql_.user.c_str(), mysql_.database.c_str(), mysql_.pass.c_str(),
            mysql_error(&mysql_.mysql));
        return false;
      }
      mysql_.isopen = true;
      return true;
    }

    int MysqlDatabaseHelper::close_mysql()
    {
      if (mysql_.isopen)
      {
        mysql_close(&mysql_.mysql);
      }
      return 0;
    }
    bool MysqlDatabaseHelper::excute_stmt(MYSQL_STMT *stmt, int64_t& mysql_proc_ret)
    {
      bool ret = true;
      int status;
      my_bool    is_null;    /* input parameter nullability */
      status = mysql_stmt_execute(stmt);
      if (status)
      {
        TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
            mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        ret = false;
      }
      if (ret)
      {
        int num_fields;       /* number of columns in result */
        MYSQL_BIND rs_bind;  /* for output buffers */

        /* the column count is > 0 if there is a result set */
        /* 0 if the result is only the final status packet */
        num_fields = mysql_stmt_field_count(stmt);
        TBSYS_LOG(DEBUG, "num_fields = %d", num_fields);
        if (num_fields == 1)
        {

          memset(&rs_bind, 0, sizeof (MYSQL_BIND));

          /* set up and bind result set output buffers */
          rs_bind.buffer_type = MYSQL_TYPE_LONG;
          rs_bind.is_null = &is_null;

          rs_bind.buffer = (char *) &mysql_proc_ret;
          rs_bind.buffer_length = sizeof(mysql_proc_ret);

          status = mysql_stmt_bind_result(stmt, &rs_bind);
          if (status)
          {
            TBSYS_LOG(ERROR, "Error: %s (errno: %d)\n",
                mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
            ret = false;
          }
          while (ret)
          {
            status = mysql_stmt_fetch(stmt);

            if (status == MYSQL_NO_DATA)
            {
              break;
            }
            if (status == 1)
            {
              TBSYS_LOG(ERROR, "mysql_stmt_fetch error");
            }
            TBSYS_LOG(DEBUG, "mysql_proc_ret = %"PRI64_PREFIX"d", mysql_proc_ret);
          }
          mysql_next_result(&mysql_.mysql); //mysql bugs, we must have this
        }
        else
        {
          TBSYS_LOG(ERROR, "num_fields = %d have debug info in prcedure?", num_fields);
          ret = false;
        }

        mysql_stmt_free_result(stmt);
        mysql_stmt_close(stmt);
      }
      return ret;
    }
  }
}

                                                                                                                                                                                                                                                                                                                                                                  sql_define.cpp                                                                                      0000644 0141723 0001014 00000000375 11613721323 012623  0                                                                                                    ustar   daoan                           daoan                                                                                                                                                                                                                  #include "sql_define.h"

namespace tfs
{
  namespace namemetaserver
  {
    std::string ConnStr::mysql_conn_str_ = "10.232.35.41:3306:tfs_name_db";
    std::string ConnStr::mysql_user_ = "root";
    std::string ConnStr::mysql_password_ = "root";
  }
}

                                                                                                                                                                                                                                                                   test.cpp                                                                                            0000644 0141723 0001014 00000012656 11613721322 011475  0                                                                                                    ustar   daoan                           daoan                                                                                                                                                                                                                  /*
* (C) 2007-2010 Alibaba Group Holding Limited.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
*
* Version: $Id
*
* Authors:
*   daoan <daoan@taobao.com>
*      - initial release
*
*/
#include "mysql_database_helper.h"
using namespace tfs;
using namespace tfs::namemetaserver;
void dump_meta_info(const MetaInfo& metainfo)
{
  int size = metainfo.size_;
  int nlen = metainfo.name_.length();

  TBSYS_LOG(INFO, "size = %d, name_len = %d", size, nlen);
}

int main()
{
  MysqlDatabaseHelper tt;
  tt.set_conn_param("10.232.35.41:3306:tfs_name_db","root","root");
  int64_t mysql_proc_ret = -1;
  char pname[512];
  char name[512];
  int ret;
  ret = tt.get_nex_val(mysql_proc_ret);
  printf("ret = %d get_nex_val = %ld-----------\n", ret, mysql_proc_ret);

  ret = tt.get_nex_val(mysql_proc_ret);
  printf("ret = %d get_nex_val = %ld-----------\n", ret, mysql_proc_ret);

  std::vector<MetaInfo> out_v_meta_info;
  ret = tt.ls_meta_info(out_v_meta_info, 2, 2);
  printf("ls_meta_info ret = %d size = %lu\n", ret , out_v_meta_info.size());
  for (size_t i = 0; i < out_v_meta_info.size(); i++)
  {
    dump_meta_info(out_v_meta_info[i]);
  }

  sprintf(pname,"%c",0);
  sprintf(name, "%cR", 1);

  ret = tt.create_dir(2, 2, 0, pname, 1, 0, 30, name, 2, mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld-----------\n", ret, mysql_proc_ret);

  sprintf(pname, "%cR", 1);
  sprintf(name,"%c%s",4,"1234");

  printf("will create dir 4_1234\n");
  ret = tt.create_dir(2, 2, 0, pname, 2, 30, 31, name, 5, mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld\n", ret, mysql_proc_ret);
  printf("will rm dir 4_1234 input\n");
  //scanf("%d", &mysql_proc_ret);

  ret = tt.rm_dir(2, 2, 0, pname, 2, 30, 31, name, 5, mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld\n", ret, mysql_proc_ret);

  printf("will create 4_1234 dir\n");
  //scanf("%d", &mysql_proc_ret);
  ret = tt.create_dir(2, 2, 0, pname, 2, 30, 31, name, 5, mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld\n", ret, mysql_proc_ret);

  printf("will create 4_1234->3_123 dir\n");
  sprintf(pname, "%c%s", 4,"1234");
  sprintf(name,"%c%s",3,"123");
  ret = tt.create_dir(2, 2, 30, pname, 5, 31, 32, name, 4, mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld\n", ret, mysql_proc_ret);

  char s_pname[256];
  char d_pname[256];
  char s_name[256];
  char d_name[256];
  sprintf(s_pname, "%c%s", 4, "1234");
  sprintf(d_pname, "%cR", 1);
  sprintf(s_name, "%c%s", 3, "123");
  sprintf(d_name, "%c%s", 2, "12");
  printf("will mv dir 4_1234->3_123  R->2_12\n");
  //scanf("%d", &mysql_proc_ret);
  ret = tt.mv_dir(2, 2,
      30, 31, s_pname, 5,
      0, 30, d_pname, 2,
      s_name, 4, d_name, 3,
      mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld\n", ret, mysql_proc_ret);

  printf("will create file 4_1234->4_file \n");
  sprintf(pname,"%c%s", 4, "1234");
  sprintf(name, "%c%s", 4, "file");
  ret = tt.create_file(2, 2,
      30, 31, pname, 5,
      name, 5,
      mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld\n", ret, mysql_proc_ret);

  ret = tt.ls_meta_info(out_v_meta_info, 2, 2);
  printf("ls_meta_info ret = %d size = %lu\n", ret , out_v_meta_info.size());
  for (size_t i = 0; i < out_v_meta_info.size(); i++)
  {
    dump_meta_info(out_v_meta_info[i]);
  }
  ret = tt.ls_meta_info(out_v_meta_info, 2, 2);
  printf("ls_meta_info ret = %d \n", ret);
  for (size_t i = 0; i < out_v_meta_info.size(); i++)
  {
    dump_meta_info(out_v_meta_info[i]);
  }

  printf("will rm file 4_1234->4_file \n");
  ret = tt.rm_file(2, 2,
      30, 31, pname, 5,
      name, 5,
      mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld\n", ret, mysql_proc_ret);

  printf("will create file 4_1234->4_file \n");
  sprintf(pname,"%c%s", 4, "1234");
  sprintf(name, "%c%s", 4, "file");
  ret = tt.create_file(2, 2,
      30, 31, pname, 5,
      name, 5,
      mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld\n", ret, mysql_proc_ret);

  printf("will pwrite file 4_1234->4_file \n");
  ret = tt.pwrite_file(2, 2,
      31, name, 5,
      100, 1,
      name,  100,
      mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld\n", ret, mysql_proc_ret);

  printf("will pwrite file 4_1234->4_file[100] \n");
  name[5]=name[6]=name[7]=0;
  name[8]=100;
  ret = tt.pwrite_file(2, 2,
      31, name, 9,
      100, 0,
      name,  100,
      mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld\n", ret, mysql_proc_ret);

  sprintf(s_pname, "%c%s", 4, "1234");
  sprintf(d_pname, "%cR", 1);
  sprintf(s_name, "%c%s", 4, "file");
  sprintf(d_name, "%c%s", 4, "elif");
  printf("will mv file 4_1234->4_file  R->4_elif\n");
  //scanf("%d", &mysql_proc_ret);
  ret = tt.mv_file(2, 2,
      30, 31, s_pname, 5,
      0, 30, d_pname, 2,
      s_name, 5, d_name, 5,
      mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld\n", ret, mysql_proc_ret);
  ret = tt.ls_meta_info(out_v_meta_info, 2, 2);
  printf("ls_meta_info ret = %d \n", ret);
  for (size_t i = 0; i < out_v_meta_info.size(); i++)
  {
    dump_meta_info(out_v_meta_info[i]);
  }
  ret = tt.ls_meta_info(out_v_meta_info, 2, 1);
  printf("ls_meta_info ret = %d size = %lu\n", ret , out_v_meta_info.size());
  for (size_t i = 0; i < out_v_meta_info.size(); i++)
  {
    dump_meta_info(out_v_meta_info[i]);
  }

  ret = tt.get_nex_val(mysql_proc_ret);
  printf("ret = %d get_nex_val= %ld-----------\n", ret, mysql_proc_ret);
  return 0;
}
                                                                                  test_meta_store_mananger.cpp                                                                        0000644 0141723 0001014 00000012656 11613721322 015567  0                                                                                                    ustar   daoan                           daoan                                                                                                                                                                                                                  /*
* (C) 2007-2010 Alibaba Group Holding Limited.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
*
* Version: $Id
*
* Authors:
*   daoan <daoan@taobao.com>
*      - initial release
*
*/
#include "mysql_database_helper.h"
using namespace tfs;
using namespace tfs::namemetaserver;
void dump_meta_info(const MetaInfo& metainfo)
{
  int size = metainfo.size_;
  int nlen = metainfo.name_.length();

  TBSYS_LOG(INFO, "size = %d, name_len = %d", size, nlen);
}

int main()
{
  MysqlDatabaseHelper tt;
  tt.set_conn_param("10.232.35.41:3306:tfs_name_db","root","root");
  int64_t mysql_proc_ret = -1;
  char pname[512];
  char name[512];
  int ret;
  ret = tt.get_nex_val(mysql_proc_ret);
  printf("ret = %d get_nex_val = %ld-----------\n", ret, mysql_proc_ret);

  ret = tt.get_nex_val(mysql_proc_ret);
  printf("ret = %d get_nex_val = %ld-----------\n", ret, mysql_proc_ret);

  std::vector<MetaInfo> out_v_meta_info;
  ret = tt.ls_meta_info(out_v_meta_info, 2, 2);
  printf("ls_meta_info ret = %d size = %lu\n", ret , out_v_meta_info.size());
  for (size_t i = 0; i < out_v_meta_info.size(); i++)
  {
    dump_meta_info(out_v_meta_info[i]);
  }

  sprintf(pname,"%c",0);
  sprintf(name, "%cR", 1);

  ret = tt.create_dir(2, 2, 0, pname, 1, 0, 30, name, 2, mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld-----------\n", ret, mysql_proc_ret);

  sprintf(pname, "%cR", 1);
  sprintf(name,"%c%s",4,"1234");

  printf("will create dir 4_1234\n");
  ret = tt.create_dir(2, 2, 0, pname, 2, 30, 31, name, 5, mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld\n", ret, mysql_proc_ret);
  printf("will rm dir 4_1234 input\n");
  //scanf("%d", &mysql_proc_ret);

  ret = tt.rm_dir(2, 2, 0, pname, 2, 30, 31, name, 5, mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld\n", ret, mysql_proc_ret);

  printf("will create 4_1234 dir\n");
  //scanf("%d", &mysql_proc_ret);
  ret = tt.create_dir(2, 2, 0, pname, 2, 30, 31, name, 5, mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld\n", ret, mysql_proc_ret);

  printf("will create 4_1234->3_123 dir\n");
  sprintf(pname, "%c%s", 4,"1234");
  sprintf(name,"%c%s",3,"123");
  ret = tt.create_dir(2, 2, 30, pname, 5, 31, 32, name, 4, mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld\n", ret, mysql_proc_ret);

  char s_pname[256];
  char d_pname[256];
  char s_name[256];
  char d_name[256];
  sprintf(s_pname, "%c%s", 4, "1234");
  sprintf(d_pname, "%cR", 1);
  sprintf(s_name, "%c%s", 3, "123");
  sprintf(d_name, "%c%s", 2, "12");
  printf("will mv dir 4_1234->3_123  R->2_12\n");
  //scanf("%d", &mysql_proc_ret);
  ret = tt.mv_dir(2, 2,
      30, 31, s_pname, 5,
      0, 30, d_pname, 2,
      s_name, 4, d_name, 3,
      mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld\n", ret, mysql_proc_ret);

  printf("will create file 4_1234->4_file \n");
  sprintf(pname,"%c%s", 4, "1234");
  sprintf(name, "%c%s", 4, "file");
  ret = tt.create_file(2, 2,
      30, 31, pname, 5,
      name, 5,
      mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld\n", ret, mysql_proc_ret);

  ret = tt.ls_meta_info(out_v_meta_info, 2, 2);
  printf("ls_meta_info ret = %d size = %lu\n", ret , out_v_meta_info.size());
  for (size_t i = 0; i < out_v_meta_info.size(); i++)
  {
    dump_meta_info(out_v_meta_info[i]);
  }
  ret = tt.ls_meta_info(out_v_meta_info, 2, 2);
  printf("ls_meta_info ret = %d \n", ret);
  for (size_t i = 0; i < out_v_meta_info.size(); i++)
  {
    dump_meta_info(out_v_meta_info[i]);
  }

  printf("will rm file 4_1234->4_file \n");
  ret = tt.rm_file(2, 2,
      30, 31, pname, 5,
      name, 5,
      mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld\n", ret, mysql_proc_ret);

  printf("will create file 4_1234->4_file \n");
  sprintf(pname,"%c%s", 4, "1234");
  sprintf(name, "%c%s", 4, "file");
  ret = tt.create_file(2, 2,
      30, 31, pname, 5,
      name, 5,
      mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld\n", ret, mysql_proc_ret);

  printf("will pwrite file 4_1234->4_file \n");
  ret = tt.pwrite_file(2, 2,
      31, name, 5,
      100, 1,
      name,  100,
      mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld\n", ret, mysql_proc_ret);

  printf("will pwrite file 4_1234->4_file[100] \n");
  name[5]=name[6]=name[7]=0;
  name[8]=100;
  ret = tt.pwrite_file(2, 2,
      31, name, 9,
      100, 0,
      name,  100,
      mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld\n", ret, mysql_proc_ret);

  sprintf(s_pname, "%c%s", 4, "1234");
  sprintf(d_pname, "%cR", 1);
  sprintf(s_name, "%c%s", 4, "file");
  sprintf(d_name, "%c%s", 4, "elif");
  printf("will mv file 4_1234->4_file  R->4_elif\n");
  //scanf("%d", &mysql_proc_ret);
  ret = tt.mv_file(2, 2,
      30, 31, s_pname, 5,
      0, 30, d_pname, 2,
      s_name, 5, d_name, 5,
      mysql_proc_ret);
  printf("ret = %d mysql_proc_ret = %ld\n", ret, mysql_proc_ret);
  ret = tt.ls_meta_info(out_v_meta_info, 2, 2);
  printf("ls_meta_info ret = %d \n", ret);
  for (size_t i = 0; i < out_v_meta_info.size(); i++)
  {
    dump_meta_info(out_v_meta_info[i]);
  }
  ret = tt.ls_meta_info(out_v_meta_info, 2, 1);
  printf("ls_meta_info ret = %d size = %lu\n", ret , out_v_meta_info.size());
  for (size_t i = 0; i < out_v_meta_info.size(); i++)
  {
    dump_meta_info(out_v_meta_info[i]);
  }

  ret = tt.get_nex_val(mysql_proc_ret);
  printf("ret = %d get_nex_val= %ld-----------\n", ret, mysql_proc_ret);
  return 0;
}
                                                                                  test_pool.cpp                                                                                       0000644 0141723 0001014 00000003461 11613721322 012520  0                                                                                                    ustar   daoan                           daoan                                                                                                                                                                                                                  /*
* (C) 2007-2010 Alibaba Group Holding Limited.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
*
* Version: $Id
*
* Authors:
*   daoan <daoan@taobao.com>
*      - initial release
*
*/
#include <pthread.h>
#include "mysql_database_helper.h"
#include "database_pool.h"
using namespace tfs;
using namespace tfs::namemetaserver;
void* run(void* arg)
{
  DataBasePool* dbp = (DataBasePool*) arg;
  int failed = 0;
  int ok = 0;
  std::vector<MetaInfo> out_v_meta_info;
  int ret;
  for (int i = 0; i < 1000; i++)
  {
    DatabaseHelper* db = dbp->get(1);
    if (NULL != db)
    {
      ok++;
      ret = db->ls_meta_info(out_v_meta_info, 2, 2);
      //TBSYS_LOG(INFO, "ls_meta_info ret = %d size = %lu\n", ret , out_v_meta_info.size());
      dbp->release(db);
    }
    else
    {
      failed++;
    }
    usleep(500);
  }
  TBSYS_LOG(INFO, "get faild %d ok = %d", failed, ok);
  pthread_exit(NULL);
}
int main()
{
  assert (1==mysql_thread_safe());
  enum {POOL_SIZE = 10,};
  DataBasePool tt(POOL_SIZE);
  char coon[50] = {"10.232.35.41:3306:tfs_name_db"};
  char usr[50] = {"root"};
  char pass[50] = {"root"};
  const char* coon_str[POOL_SIZE];
  const char* usr_name[POOL_SIZE];
  const char* pass_word[POOL_SIZE];
  int32_t hash_flag[POOL_SIZE];
  for (int i = 0; i < POOL_SIZE; i++)
  {
    coon_str[i] = coon;
    usr_name[i] = usr;
    pass_word[i]= pass;
    hash_flag[i] = 1;
  }

  assert(tt.init_pool(coon_str, usr_name, pass_word, hash_flag));

  pthread_t tid[30];
  for (int i = 0; i < 20; i++)
  {
    assert(0 == pthread_create(&tid[i], NULL, run, &tt));
  }
  for (int i = 0; i < 20; i++)
  {
    pthread_join(tid[i], NULL);
  }
  TBSYS_LOG(INFO, "all thread end");
  return 0;
}
                                                                                                                                                                                                               test_service.cpp                                                                                    0000644 0141723 0001014 00000015556 11613721322 013217  0                                                                                                    ustar   daoan                           daoan                                                                                                                                                                                                                  /*
* (C) 2007-2010 Alibaba Group Holding Limited.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
*
* Version: $Id
*
* Authors:
*   daoan <daoan@taobao.com>
*      - initial release
*
*/
#include "meta_server_service.h"
using namespace tfs;
using namespace tfs::namemetaserver;
void dump_meta_info(const MetaInfo& metainfo)
{
  int size = metainfo.size_;
  int nlen = metainfo.name_.length();

  TBSYS_LOG(INFO, "size = %d, name_len = %d", size, nlen);
}

int main()
{
  int64_t app_id = 3;
  int64_t uid = 1;

  char dir_path[512], new_dir_path[512], wrong_dir_path[512];
  char file_path[512], new_file_path[512], wrong_file_path[512];
  int ret = 1;

  tfs::namemetaserver::MetaServerService service;

  // create file or dir test
  // TODO read file to test if the file or dir is exist
  printf("-----------------------------------------------\n");
  printf("app_id %lu, uid: %lu\n", app_id, uid);

  sprintf(dir_path, "/test");

  ret = service.create(app_id, uid, dir_path, DIRECTORY);
  printf("create dir %s, ret: %d\n", dir_path, ret);

  ret = service.create(app_id, uid, dir_path, DIRECTORY);
  printf("create dir %s, ret: %d\n", dir_path, ret);

  sprintf(file_path, "/test/1.txt");

  ret = service.create(app_id, uid, file_path, NORMAL_FILE);
  printf("create file %s, ret: %d\n", file_path, ret);

  ret = service.create(app_id, uid, file_path, NORMAL_FILE);
  printf("create file %s, ret: %d\n", file_path, ret);

  sprintf(wrong_dir_path, "/admin/test");
  ret = service.create(app_id, uid, wrong_dir_path, DIRECTORY);
  printf("create dir %s, ret: %d\n", wrong_dir_path, ret);

  sprintf(wrong_file_path, "/admin/test/1.txt");
  ret = service.create(app_id, uid, wrong_file_path, NORMAL_FILE);
  printf("create file %s, ret: %d\n", wrong_file_path, ret);

  // rm file or dir test
  // TODO read file to test if the file or dir is exist
  uid = uid + 1;
  printf("-----------------------------------------------\n");
  printf("app_id %lu, uid: %lu\n", app_id, uid);

  sprintf(dir_path, "/test");
  ret = service.create(app_id, uid, dir_path, DIRECTORY);
  printf("create dir %s, ret: %d\n", dir_path, ret);
  ret = service.rm(app_id, uid, dir_path, DIRECTORY);
  printf("rm dir %s, ret: %d\n", dir_path, ret);

  sprintf(dir_path, "/test");
  ret = service.create(app_id, uid, dir_path, DIRECTORY);
  sprintf(file_path, "/test/1.txt");
  ret = service.create(app_id, uid, file_path, NORMAL_FILE);
  printf("create file %s, ret: %d\n", file_path, ret);
  ret = service.rm(app_id, uid, dir_path, DIRECTORY);
  printf("rm dir %s, ret: %d\n", dir_path, ret);
  ret = service.rm(app_id, uid, file_path, NORMAL_FILE);
  printf("rm file %s, ret: %d\n", file_path, ret);
  ret = service.rm(app_id, uid, dir_path, DIRECTORY);
  printf("rm dir %s, ret: %d\n", dir_path, ret);

  sprintf(wrong_dir_path, "/admin/test");
  ret = service.rm(app_id, uid, wrong_dir_path, DIRECTORY);
  printf("rm dir %s, ret: %d\n", wrong_file_path, ret);

  sprintf(wrong_file_path, "/admin/test/1.txt");
  ret = service.rm(app_id, uid, wrong_file_path, NORMAL_FILE);
  printf("rm file %s, ret: %d\n", wrong_file_path, ret);

  // mv file or dir test

  // --directory to directory 
  uid = uid + 1;
  printf("-----------------------------------------------\n");
  printf("app_id %lu, uid: %lu\n", app_id, uid);

  sprintf(dir_path, "/test");
  ret = service.create(app_id, uid, dir_path, DIRECTORY);
  printf("create dir %s, %d\n", dir_path, ret);

  sprintf(new_dir_path, "/admin");
  ret = service.create(app_id, uid, new_dir_path, DIRECTORY);
  printf("create dir %s, %d\n", new_dir_path, ret);

  ret = service.mv(app_id, uid, dir_path, new_dir_path, DIRECTORY);
  printf("mv file %s->%s, %d\n", dir_path, new_dir_path, ret);

  ret = service.rm(app_id, uid, new_dir_path, DIRECTORY);
  printf("rm dir %s, %d\n", new_dir_path, ret);

  ret = service.mv(app_id, uid, dir_path, new_dir_path, DIRECTORY);
  printf("mv file %s->%s, %d\n", dir_path, new_dir_path, ret);

  // --file to file
  uid = uid + 1;
  printf("app_id %lu, uid: %lu\n", app_id, uid);
  
  sprintf(dir_path, "/test");
  ret = service.create(app_id, uid, dir_path, DIRECTORY);
  printf("create dir %s, %d\n", dir_path, ret);
  sprintf(file_path, "/test/old.txt");
  ret = service.create(app_id, uid, file_path, NORMAL_FILE);
  printf("create file %s, %d\n", file_path, ret);

  sprintf(new_dir_path, "/admin");
  ret = service.create(app_id, uid, new_dir_path, DIRECTORY);
  printf("create dir %s, %d\n", new_dir_path, ret);
  sprintf(new_file_path, "/admin/new.txt");
  ret = service.create(app_id, uid, new_file_path, NORMAL_FILE);
  printf("create file %s, %d\n", new_file_path, ret);

  ret = service.mv(app_id, uid, file_path, new_file_path, NORMAL_FILE);
  printf("mv file %s->%s, %d\n", file_path, new_file_path, ret);

  ret = service.rm(app_id, uid, new_file_path, NORMAL_FILE);
  printf("rm dir %s, %d\n", new_file_path, ret);

  ret = service.mv(app_id, uid, file_path, new_file_path, NORMAL_FILE);
  printf("mv dir %s->%s, %d\n", file_path, new_file_path, ret);

  // --file to directory 
  uid = uid + 1;
  printf("app_id %lu, uid: %lu\n", app_id, uid);

  sprintf(dir_path, "/test");
  ret = service.create(app_id, uid, dir_path, DIRECTORY);
  printf("create dir %s, %d\n", dir_path, ret);
  sprintf(file_path, "/test/old.txt");
  ret = service.create(app_id, uid, file_path, NORMAL_FILE);
  printf("create file %s, %d\n", file_path, ret);

  sprintf(new_dir_path, "/admin");
  ret = service.create(app_id, uid, new_dir_path, DIRECTORY);
  printf("create dir %s, %d\n", new_file_path, ret);

  ret = service.mv(app_id, uid, file_path, new_dir_path, NORMAL_FILE);
  printf("mv file %s->%s, %d\n", file_path, new_dir_path, ret);

  ret = service.rm(app_id, uid, new_dir_path, DIRECTORY);
  printf("rm dir %s, %d\n", new_dir_path, ret);

  ret = service.mv(app_id, uid, file_path, new_dir_path, NORMAL_FILE);
  printf("mv file %s->%s, %d\n", file_path, new_dir_path, ret);

  // --directory to file 
  uid = uid + 1;
  printf("app_id %lu, uid: %lu\n", app_id, uid);

  sprintf(dir_path, "/test");
  ret = service.create(app_id, uid, dir_path, DIRECTORY);
  printf("create dir %s, %d\n", dir_path, ret);

  sprintf(new_dir_path, "/admin");
  ret = service.create(app_id, uid, new_dir_path, DIRECTORY);
  printf("create dir %s, %d\n", new_file_path, ret);
  sprintf(new_file_path, "/admin/new.txt");
  ret = service.create(app_id, uid, new_file_path, NORMAL_FILE);
  printf("create file %s, %d\n", new_file_path, ret);

  ret = service.mv(app_id, uid, dir_path, new_file_path, DIRECTORY);
  printf("mv file %s->%s, %d\n", dir_path, new_file_path, ret);

  ret = service.rm(app_id, uid, new_file_path, NORMAL_FILE);
  printf("rm dir %s, %d\n", new_file_path, ret);

  ret = service.mv(app_id, uid, dir_path, new_file_path, DIRECTORY);
  printf("mv file %s->%s, %d\n", dir_path, new_file_path, ret);

  return 0;
}
                                                                                                                                                  database_helper.h                                                                                   0000644 0141723 0001014 00000007007 11613721322 013260  0                                                                                                    ustar   daoan                           daoan                                                                                                                                                                                                                  /*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id
 *
 *
 *   Authors:
 *          daoan(daoan@taobao.com)
 *
 */

#ifndef TFS_NAMEMETASERVER_DATABASE_HELPER_H_
#define TFS_NAMEMETASERVER_DATABASE_HELPER_H_
#include "common/internal.h"
#include "meta_info.h"
namespace tfs
{
  namespace namemetaserver
  {
    class DatabaseHelper
    {
      public:
        DatabaseHelper();
        virtual ~DatabaseHelper();

        bool is_connected() const;
        int set_conn_param(const char* conn_str, const char* user_name, const char* passwd);

        virtual int connect() = 0;
        virtual int close() = 0;

        virtual int ls_meta_info(std::vector<MetaInfo>& out_v_meta_info,
            const int64_t app_id, const int64_t uid,
            const int64_t pid = 0, const char* name = NULL, const int32_t name_len = 0) = 0;

        virtual int create_dir(const int64_t app_id, const int64_t uid,
            const int64_t ppid, const char* pname, const int32_t pname_len,
            const int64_t pid, const int64_t id, const char* name, const int32_t name_len,
            int64_t& mysql_proc_ret) = 0;

        virtual int rm_dir(const int64_t app_id, const int64_t uid, const int64_t ppid,
            const char* pname, const int32_t pname_len, const int64_t pid, const int64_t id,
            const char* name, const int32_t name_len, int64_t& mysql_proc_ret) = 0;

        virtual int mv_dir(const int64_t app_id, const int64_t uid,
            const int64_t s_ppid, const int64_t s_pid, const char* s_pname, const int32_t s_pname_len,
            const int64_t d_ppid, const int64_t d_pid, const char* d_pname, const int32_t d_pname_len,
            const char* s_name, const int32_t s_name_len,
            const char* d_name, const int32_t d_name_len, int64_t& mysql_proc_ret) = 0;

        virtual int create_file(const int64_t app_id, const int64_t uid,
            const int64_t ppid, const int64_t pid, const char* pname, const int32_t pname_len,
            const char* name, const int32_t name_len, int64_t& mysql_proc_ret) = 0;

        virtual int rm_file(const int64_t app_id, const int64_t uid,
            const int64_t ppid, const int64_t pid, const char* pname, const int32_t pname_len,
            const char* name, const int32_t name_len, int64_t& mysql_proc_ret) = 0;

        virtual int pwrite_file(const int64_t app_id, const int64_t uid,
            const int64_t pid, const char* name, const int32_t name_len,
            const int64_t size, const int16_t ver_no, const char* meta_info, const int32_t meta_len,
            int64_t& mysql_proc_ret) = 0;

        virtual int mv_file(const int64_t app_id, const int64_t uid,
            const int64_t s_ppid, const int64_t s_pid, const char* s_pname, const int32_t s_pname_len,
            const int64_t d_ppid, const int64_t d_pid, const char* d_pname, const int32_t d_pname_len,
            const char* s_name, const int32_t s_name_len,
            const char* d_name, const int32_t d_name_len, int64_t& mysql_proc_ret) = 0;

        virtual int get_nex_val(int64_t& next_val) = 0;

      protected:
        enum
        {
          CONN_STR_LEN = 256,
        };
        char conn_str_[CONN_STR_LEN];
        char user_name_[CONN_STR_LEN];
        char passwd_[CONN_STR_LEN];
        bool is_connected_;

      private:
        DISALLOW_COPY_AND_ASSIGN(DatabaseHelper);
    };
  }
}
#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         database_pool.h                                                                                     0000644 0141723 0001014 00000002615 11613721322 012752  0                                                                                                    ustar   daoan                           daoan                                                                                                                                                                                                                  /*
* (C) 2007-2010 Alibaba Group Holding Limited.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
*
* Version: $Id
*
* Authors:
*   daoan <daoan@taobao.com>
*      - initial release
*
*/
#ifndef TFS_NAMEMETASERVER_DATABASE_POOL_H_
#define TFS_NAMEMETASERVER_DATABASE_POOL_H_
#include <tbsys.h>
#include <Mutex.h>
#include "common/internal.h"
namespace tfs
{
  namespace namemetaserver
  {
    class DatabaseHelper;
    class DataBasePool
    {
      public:
        explicit DataBasePool(const int32_t pool_size);
        ~DataBasePool();
        bool init_pool(const char** conn_str, const char** user_name,
            const char** passwd, const int32_t* hash_flag);
        DatabaseHelper* get(const int32_t hash_flag);
        void release(DatabaseHelper* database_helper);
      private:
        enum
        {
          MAX_POOL_SIZE = 20,
        };
        struct DataBaseInfo
        {
          DataBaseInfo():database_helper_(NULL), busy_flag_(true), hash_flag_(-1) {}
          DatabaseHelper* database_helper_;
          bool busy_flag_;
          int32_t hash_flag_;
        };
        DataBaseInfo base_info_[MAX_POOL_SIZE];
        int32_t pool_size_;
        tbutil::Mutex mutex_;
      private:
        DISALLOW_COPY_AND_ASSIGN(DataBasePool);
    };
  }
}
#endif
                                                                                                                   meta_info.h                                                                                         0000644 0141723 0001014 00000011266 11613737125 012130  0                                                                                                    ustar   daoan                           daoan                                                                                                                                                                                                                  /*
* (C) 2007-2010 Alibaba Group Holding Limited.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
*
* Version: $Id
*
* Authors:
*   daoan <daoan@taobao.com>
*      - initial release
*
*/

#ifndef TFS_NAMEMETASERVER_METAINFO_H_
#define TFS_NAMEMETASERVER_METAINFO_H_
#include <vector>
#include <string>
#include "common/serialization.h"

namespace tfs
{
  namespace namemetaserver 
  {
    struct FragMeta
    {
      int64_t offset_;
      uint64_t file_id_;
      int32_t size_;
      uint32_t block_id_;
      static int64_t get_length() const
      {
        return 2 * sizeof(int64_t) + 2 * sizeof(int32_t);
      }
      int serialize(char* data, const int64_t buff_len, int64_t& pos) const
      {
        int ret = TFS_ERROR;
        if (buff_len - pos >= get_length())
        {
          common::Serialization::set_int64(data, buff_len, pos, offset_);
          common::Serialization::set_int64(data, buff_len, pos, file_id_);
          common::Serialization::set_int32(data, buff_len, pos, size_);
          common::Serialization::set_int32(data, buff_len, pos, block_id_);
          ret = TFS_SUCCESS;
        }
        return ret;
      }
      int deserialize(char* data, const int64_t data_len, int64_t& pos)
      {
        int ret = get_int64(data, data_len, pos, &offset_);
        if (TFS_SUCCESS == ret)
        {
          ret = get_int64(data, data_len, pos, &file_id_);
        }
        if (TFS_SUCCESS == ret)
        {
          ret = get_int32(data, data_len, pos, &size_);
        }
        if (TFS_SUCCESS == ret)
        {
          ret = get_int32(data, data_len, pos, &block_id_);
        }
        return ret;
      }
      bool operator < (const FragMeta& right) const
      {
        return offset_ < right.offset_;
      }
    };
    class FragInfo
    {
      public:
      FragInfo():
        cluster_id_(0), had_been_split_(false)
      {
      }
      explicit FragInfo(const std::vector<FragMeta>& v_frag_meta):
        cluster_id_(0), had_been_split_(false),
        v_frag_meta_(v_frag_meta)
      {
      }
      int32_t cluster_id_;
      bool had_been_split_;
      std::vector<FragMeta> v_frag_meta_;
      int64_t get_length() const
      {
        return sizeof(int32_t) * 2 + v_frag_meta_.size() * FragMeta::get_length();
      }
      int serialize(char* data, const int64_t buff_len, int64_t& pos) const
      {
        int ret = TFS_ERROR;
        if (buff_len - pos >= get_length())
        {
          int32_t frag_count = static_cast<int32>(v_frag_meta_.size());
          assert(frag_count  > 0 );
          if (had_been_split_)
          {
            frag_count |= (1 << 31);
          }
          common::Serialization::set_int32(data, buff_len, pos, frag_count);
          common::Serialization::set_int32(data, buff_len, pos, cluster_id_);
          std::vector<FragMeta>::const_iterator iter = v_frag_meta_.begin();
          for (; iter != v_frag_meta_.end(); iter++)
          {
            (*iter).serialize(data, buff_len, pos);
          }
          ret = TFS_SUCCESS;
        }
        return ret;
      }
      int deserialize(char* data, const int64_t data_len, int64_t& pos)
      {
        int32_t frg_count = 0;
        int ret = get_int32(data, data_len, pos, &frg_count);
        if (TFS_SUCCESS == ret)
        {
          had_been_split_ = (frag_count >> 31);
          frg_count = frg_count & ~(1 << 31)
          ret = get_int32(data, data_len, pos, &cluster_id_);
        }
        if (TFS_SUCCESS == ret)
        {
          v_frag_meta_.clear();
          FragMeta tmp;
          for (int i = 0; i < frg_count && TFS_SUCCESS == ret; i ++)
          {
            ret = tmp.deserialize(data, data_len, pos);
            v_frag_meta_.push_back(tmp);
          }
        }
        return ret;
      }
      int64_t get_last_offset() const
      {
        int64_t last_offset = 0;
        if (!v_frag_meta_.empty())
        {
          std::vector<FragMeta>::const_iterator it = v_frag_meta_.end() - 1;
          last_offset = it->offset_ + it->size_;
        }
        return last_offset;
      }
    };
    class MetaInfo
    {
      public:
        MetaInfo():
          pid_(-1), id_(0), create_time_(0), modify_time_(0),
          size_(0), ver_no_(0)
      {
      }
        MetaInfo(FragInfo frag_info):
          pid_(-1), id_(0), create_time_(0), modify_time_(0),
          size_(0), ver_no_(0), frag_info_(frag_info)
      {
      }

        int64_t pid_;
        int64_t id_;
        int32_t create_time_;
        int32_t modify_time_;
        int64_t size_;
        int16_t ver_no_;
        std::string name_;
        FragInfo frag_info_;
    };
  }
}

#endif
                                                                                                                                                                                                                                                                                                                                          meta_server_service.h                                                                               0000644 0141723 0001014 00000011101 11613745425 014211  0                                                                                                    ustar   daoan                           daoan                                                                                                                                                                                                                  /*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id
 *
 * Authors:
 *   nayan <nayan@taobao.com>
 *      - initial release
 *
 */

#ifndef TFS_NAMEMETASERVER_NAMEMETASERVERSERVICE_H_
#define TFS_NAMEMETASERVER_NAMEMETASERVERSERVICE_H_

#include "common/base_service.h"
#include "message/message_factory.h"
#include "meta_store_manager.h"

namespace tfs
{
  namespace namemetaserver
  {
    const int32_t MAX_FILE_PATH_LEN = 512;
    const int32_t INSERT_VERSION_ERROR = -1045;
    const int32_t UPDATE_FRAG_INFO_ERROR = -1046;
    const int32_t SOFT_MAX_FRAG_INFO_COUNT = 1024;
    const int32_t MAX_FRAG_INFO_SIZE = 65535;
    const int32_t MAX_OUT_FRAG_INFO = 256;

    class MetaServerService : public common::BaseService
    {
    public:
      MetaServerService();
      ~MetaServerService();

    public:
      // override
      virtual tbnet::IPacketStreamer* create_packet_streamer();
      virtual void destroy_packet_streamer(tbnet::IPacketStreamer* streamer);
      virtual common::BasePacketFactory* create_packet_factory();
      virtual void destroy_packet_factory(common::BasePacketFactory* factory);
      virtual const char* get_log_file_path();
      virtual const char* get_pid_file_path();
      virtual bool handlePacketQueue(tbnet::Packet* packet, void* args);

      int create(const int64_t app_id, const int64_t uid,
          const char* file_path, const FileType type);
      int rm(const int64_t app_id, const int64_t uid,
          const char* file_path, const FileType type);
      int mv(const int64_t app_id, const int64_t uid,
          const char* file_path, const char* dest_file_path, const FileType type);
      int write(const int64_t app_id, const int64_t uid,
          const char* file_path, const int32_t cluster_id,
          const std::vector<FragMeta>& v_frag_info, int32_t* write_ret);
      int read(const int64_t app_id, const int64_t uid, const char* file_path,
          const int64_t offset, const int64_t size,
          int32_t& cluster_id, std::vector<FragMeta>& v_frag_info, bool& still_have );

    private:
      // override
      virtual int initialize(int argc, char* argv[]);
      virtual int destroy_service();

      int get_p_meta_info(const int64_t app_id, const int64_t uid, 
          const std::vector<std::string>& v_name,
          MetaInfo& out_meta_info, int32_t& name_len);
      int create_top_dir(const int64_t app_id, const int64_t uid);

      static int parse_name(const char* file_path, std::vector<std::string>& v_name);
      static int32_t get_depth(const std::vector<std::string>& v_name);
      static int get_name(const std::vector<std::string>& v_name, const int32_t depth, char* buffer, const int32_t buffer_len, int32_t& name_len);


      int get_meta_info(const int64_t app_id, const int64_t uid, const int64_t pid,
          const char* name, const int32_t name_len, const int64_t offset,
          std::vector<MetaInfo>& v_meta_info);

      int read_frag_info(const MetaInfo& v_meta_info, const int64_t pid,
          const char* name, const int32_t name_len, const int64_t offset,
          const int64_t size, int32_t& cluster_id,
          std::vector<FragMeta>& v_out_fraginfo, bool& still_have);

      int write_frag_info(int32_t cluster_id, std::vector<MetaInfo>& v_meta_info,
          const std::vector<FragMeta>& v_frag_info);
      int insert_frag_info(std::vector<MetaInfo>& v_meta_info, const std::vector<FragMeta>& src_v_frag_info,
          int32_t& end_index);
      int merge_frag_info(const MetaInfo& meta_info, const std::vector<FragMeta>& src_v_frag_info,
          int32_t& end_index);
      int fast_merge_frag_info(std::vector<FragMeta>& dest_v_frag_info,
          const std::vector<FragMeta>& dest_v_frag_info,
          std::vector<FragMeta>::const_iterator& src_it,
          int32_t& end_index);
      int update_write_frag_info(std::vector<MetaInfo>& v_meta_info, const int64_t app_id, const int64_t uid,
          MetaInfo& p_meta_info, const char* panme, const int32_t pname_len,
          char* name, const int32_t name_len);
      std::vector<FragMeta>::const_iterator
        lower_find(const std::vector<FragMeta>& v_frag_info, const int64_t offset);
      std::vector<FragMeta>::const_iterator
        upper_find(const std::vector<FragMeta>& v_frag_info, const int64_t offset);

    private:
      DISALLOW_COPY_AND_ASSIGN(MetaServerService);

      MetaStoreManager* store_manager_;
      tbutil::Mutex mutex_;
    };
  }
}

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                               meta_store_manager.h                                                                                0000644 0141723 0001014 00000003670 11613721322 014013  0                                                                                                    ustar   daoan                           daoan                                                                                                                                                                                                                  /*
* (C) 2007-2010 Alibaba Group Holding Limited.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
*
* Version: $Id
*
* Authors:
*   chuyu <chuyu@taobao.com>
*      - initial release
*
*/
#ifndef TFS_NAMEMETASERVER_NAMEMETASTOREMANAGER_H_
#define TFS_NAMEMETASERVER_NAMEMETASTOREMANAGER_H_
#include "Memory.hpp"
#include "sql_define.h"
#include "meta_info.h"
#include "mysql_database_helper.h"
//#include "meta_cache_handler.h"

namespace tfs
{
  namespace namemetaserver
  {
    class MetaStoreManager
    {
      public:
        MetaStoreManager();
        ~MetaStoreManager();

        int select(const int64_t app_id, const int64_t uid, 
            const int64_t pid, const char* name, const int32_t name_len, std::vector<MetaInfo>& out_v_meta_info);

        int insert(const int64_t app_id, const int64_t uid,
            const int64_t ppid, const char* pname, const int32_t pname_len, 
            const int64_t pid, const char* name, const int32_t name_len, 
            const FileType type, MetaInfo* meta_info = NULL);

        int update(const int64_t app_id, const int64_t uid, 
            const int64_t s_ppid, const int64_t s_pid, const char* s_pname, const int32_t s_pname_len,
            const int64_t d_ppid, const int64_t d_pid, const char* d_pname, const int32_t d_pname_len,
            const char* s_name, const int32_t s_name_len,
            const char* d_name, const int32_t d_name_len,
            const FileType type);

        int remove(const int64_t app_id, const int64_t uid, const int64_t ppid,
            const char* pname, const int64_t pname_len, const int64_t pid, const int64_t id,
            const char* name, const int64_t name_len,
            const FileType type);
      private:
        DatabaseHelper* database_helper_;
        //MetaCacheHandler* meta_cache_handler_;
    };
  }
}

#endif
                                                                        mysql_database_helper.h                                                                             0000644 0141723 0001014 00000010056 11613721322 014503  0                                                                                                    ustar   daoan                           daoan                                                                                                                                                                                                                  /*
* (C) 2007-2010 Alibaba Group Holding Limited.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
*
* Version: $Id
*
* Authors:
*   daoan <daoan@taobao.com>
*      - initial release
*
*/
#ifndef TFS_NAMEMETASERVER_MYSQL_DATABASE_HELPER_H_
#define TFS_NAMEMETASERVER_MYSQL_DATABASE_HELPER_H_
#include <mysql/mysql.h>
#include <tbsys.h>
#include <Mutex.h>
#include "database_helper.h"
namespace tfs
{
  namespace namemetaserver
  {
    class MysqlDatabaseHelper :public DatabaseHelper
    {
      public:
        MysqlDatabaseHelper();
        virtual ~MysqlDatabaseHelper();
        virtual int connect();
        virtual int close();

        virtual int ls_meta_info(std::vector<MetaInfo>& out_v_meta_info,
            const int64_t app_id, const int64_t uid,
            const int64_t pid = 0, const char* name = NULL, const int32_t name_len = 0);

        virtual int create_dir(const int64_t app_id, const int64_t uid,
            const int64_t ppid, const char* pname, const int32_t pname_len,
            const int64_t pid, const int64_t id, const char* name, const int32_t name_len,
            int64_t& mysql_proc_ret);

        virtual int rm_dir(const int64_t app_id, const int64_t uid, const int64_t ppid,
            const char* pname, const int32_t pname_len, const int64_t pid, const int64_t id,
            const char* name, const int32_t name_len, int64_t& mysql_proc_ret);

        virtual int mv_dir(const int64_t app_id, const int64_t uid,
            const int64_t s_ppid, const int64_t s_pid, const char* s_pname, const int32_t s_pname_len,
            const int64_t d_ppid, const int64_t d_pid, const char* d_pname, const int32_t d_pname_len,
            const char* s_name, const int32_t s_name_len,
            const char* d_name, const int32_t d_name_len, int64_t& mysql_proc_ret);

        virtual int create_file(const int64_t app_id, const int64_t uid,
            const int64_t ppid, const int64_t pid, const char* pname, const int32_t pname_len,
            const char* name, const int32_t name_len, int64_t& mysql_proc_ret);

        virtual int rm_file(const int64_t app_id, const int64_t uid,
            const int64_t ppid, const int64_t pid, const char* pname, const int32_t pname_len,
            const char* name, const int32_t name_len, int64_t& mysql_proc_ret);

        virtual int pwrite_file(const int64_t app_id, const int64_t uid,
            const int64_t pid, const char* name, const int32_t name_len,
            const int64_t size, const int16_t ver_no, const char* meta_info, const int32_t meta_len,
            int64_t& mysql_proc_ret);

        virtual int mv_file(const int64_t app_id, const int64_t uid,
            const int64_t s_ppid, const int64_t s_pid, const char* s_pname, const int32_t s_pname_len,
            const int64_t d_ppid, const int64_t d_pid, const char* d_pname, const int32_t d_pname_len,
            const char* s_name, const int32_t s_name_len,
            const char* d_name, const int32_t d_name_len, int64_t& mysql_proc_ret);

        virtual int get_nex_val(int64_t& next_val);

      private:
        enum
        {
          ROW_LIMIT = 500,
          META_NAME_LEN = 512,
          SLIDE_INFO_LEN = 65535,
        };
        MYSQL_STMT *stmt_;
        MYSQL_BIND ps_params_[4];  /* input parameter buffers */
        int64_t app_id_;
        int64_t uid_;
        int64_t pid_;
        char pname_[META_NAME_LEN];
        unsigned long pname_len_;
      private:
        struct mysql_ex {
          std::string host;
          int port;
          std::string user;
          std::string pass;
          std::string database;
          bool   isopen;
          bool   inited;
          MYSQL  mysql;
        };

        bool init_mysql(const char* mysqlconn, const char* user_name, const char* passwd);
        bool open_mysql();
        int close_mysql();
        bool excute_stmt(MYSQL_STMT *stmt, int64_t& mysql_proc_ret);

      private:
        mysql_ex  mysql_;
        tbutil::Mutex mutex_;

    };

  }
}
#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  sql_define.h                                                                                        0000644 0141723 0001014 00000001213 11613721322 012257  0                                                                                                    ustar   daoan                           daoan                                                                                                                                                                                                                  #ifndef TFS_NAMEMETASERVER_SQLDEFINE_H_
#define TFS_NAMEMETASERVER_SQLDEFINE_H_

#include <string>
namespace tfs
{
  namespace namemetaserver
  {
    struct ConnStr
    {
      static std::string mysql_conn_str_;
      static std::string mysql_user_;
      static std::string mysql_password_;
    };

    enum FileType
    {
      NORMAL_FILE = 1,
      DIRECTORY = 2,
      PWRITE_FILE = 3
    };

    enum StoreErrorCode 
    {
      UNKNOWN_ERROR = -1,
      TARGET_EXIST_ERROR = -2,
      PARENT_NOT_EXIST_ERROR = -3,
      TARGET_NOT_EXIST_ERROR = -4,
      DELETE_DIR_WITH_FILE_ERROR = -5,
      VERSION_CONFLICT_ERROR = -6
    };
  }
}

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     