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
 *
 */
#include "replicate_block.h"
#include <Time.h>
#include <Memory.hpp>

namespace tfs
{
  namespace dataserver
  {
    using namespace common;
    using namespace message;
    using namespace tbutil;

    ReplicateBlock::ReplicateBlock(Mutex* mutex, Client* client)
    {
      init();
      client_mutex_ = mutex;
      client_ = client;
    }

    ReplicateBlock::~ReplicateBlock()
    {
    }

    void ReplicateBlock::stop()
    {
      stop_ = 1;
      clear_cloned_block_map();
      repl_block_monitor_.lock();
      repl_block_monitor_.notifyAll();
      repl_block_monitor_.unlock();
    }

    void ReplicateBlock::init()
    {
      expire_cloned_interval_ = SYSPARAM_DATASERVER.expire_cloned_block_time_;
      last_expire_cloned_block_time_ = 0;
      stop_ = 0;
      client_mutex_ = NULL;
      client_ = NULL;
    }

    void ReplicateBlock::run(tbsys::CThread* thread, void* args)
    {
      TBSYS_LOG(INFO, "tid: %u", Func::gettid());
      ReplicateBlock* ds = reinterpret_cast<ReplicateBlock *> (args);
      ds->run_replicate_block();
    }

    int ReplicateBlock::run_replicate_block()
    {
      Time timeout = Time::seconds(1);
      while (!stop_)
      {
        repl_block_monitor_.lock();
        while (!stop_)
        {
          repl_block_monitor_.timedWait(timeout);
          // wait the first one
          if (!repl_block_queue_.empty())
          {
            break;
          }
        }

        if (stop_)
        {
          repl_block_monitor_.unlock();
          break;
        }

        TBSYS_LOG(INFO, "repl block blockid: %d", repl_block_queue_.front()->block_id_);
        ReplBlock *b = repl_block_queue_.front();
        repl_block_queue_.pop_front();
        replicating_block_map_[b->block_id_] = b;
        repl_block_monitor_.unlock();

        //replicate
        int64_t start_time = Func::curr_time();
        int ret = replicate_block_to_server(b);
        if (TFS_ERROR == send_repl_block_complete_info(ret, b))
        {
          //retry send
          if (TFS_ERROR == send_repl_block_complete_info(ret, b))
          {
            TBSYS_LOG(INFO, "send ReplBlock completeinfo fail: %u", b->block_id_);
          }
        }

        int64_t end_time = Func::curr_time();
        TBSYS_LOG(INFO, "replicate %s blockid: %u, %s=>%s, cost time: %d (ms)", (ret ? "fail" : "success"),
            b->block_id_, tbsys::CNetUtil::addrToString(b->source_id_).c_str(), tbsys::CNetUtil::addrToString(
                b->destination_id_).c_str(), (end_time - start_time) / 1000);

        repl_block_monitor_.lock();
        replicating_block_map_.erase(b->block_id_);
        repl_block_monitor_.unlock();

        tbsys::gDelete(b);
      }

      repl_block_monitor_.lock();
      while (!repl_block_queue_.empty())
      {
        ReplBlock* b = repl_block_queue_.front();
        repl_block_queue_.pop_front();
        tbsys::gDelete(b);
      }

      repl_block_monitor_.unlock();
      return TFS_SUCCESS;
    }

    int ReplicateBlock::send_repl_block_complete_info(const int status, const ReplBlock* b)
    {
      ReplicateBlockMessage req_rb_msg;
      int ret = TFS_ERROR;

      req_rb_msg.set_repl_block(b);
      if (TFS_SUCCESS == status)
      {
        req_rb_msg.set_command(COMMAND_REPL_COMPLETE);
      }
      else
      {
        req_rb_msg.set_command(COMMAND_REPL_FAILURE);
      }

      int32_t need_remove = 0;
      client_mutex_->lock();
      Message* rsp_msg = client_->call(&req_rb_msg);
      if (NULL != rsp_msg)
      {
        StatusMessage* sm = dynamic_cast<StatusMessage*> (rsp_msg);
        if (b->is_move_ == REPLICATE_BLOCK_MOVE_FLAG_YES && STATUS_MESSAGE_REMOVE == sm->get_status())
        {
          need_remove = 1;
          ret = TFS_SUCCESS;
        }
        else if (STATUS_MESSAGE_OK == sm->get_status())
        {
          ret = TFS_SUCCESS;
        }
        else
        {
          TBSYS_LOG(ERROR, "send repl block complete info: %s\n", sm->get_error());
        }
        tbsys::gDelete(rsp_msg);
      }

      client_mutex_->unlock();
      if (need_remove)
      {
        int rm_ret = BlockFileManager::get_instance()->del_block(b->block_id_);
        TBSYS_LOG(INFO, "send repl block complete info: del blockid: %u, result: %d\n", b->block_id_, rm_ret);
      }
      return ret;
    }

    // replicate one block to other ds
    int ReplicateBlock::replicate_block_to_server(const ReplBlock* b)
    {
      uint64_t ds_ip = b->destination_id_;
      uint32_t block_id = b->block_id_;

      TBSYS_LOG(INFO, "replicating now, blockid: %u, %s = >%s\n", b->block_id_, tbsys::CNetUtil::addrToString(
          b->source_id_).c_str(), tbsys::CNetUtil::addrToString(b->destination_id_).c_str());

      LogicBlock* logic_block = BlockFileManager::get_instance()->get_logic_block(block_id);
      if (NULL == logic_block)
      {
        TBSYS_LOG(ERROR, "block is not exist, blockid: %u, %s=>%s\n", b->block_id_, tbsys::CNetUtil::addrToString(
            b->source_id_).c_str(), tbsys::CNetUtil::addrToString(b->destination_id_).c_str());
        return TFS_ERROR;
      }

      // send to port + 1
      ds_ip = Func::addr_inc_port(ds_ip, 1);
      Client* client = CLIENT_POOL.get_client(ds_ip);
      if (NULL == client)
      {
        TBSYS_LOG(ERROR, "CLIENT_POOL get client ds_ip '%s' fail. blockid: %u\n",
            tbsys::CNetUtil::addrToString(ds_ip).c_str(), block_id);
        return TFS_ERROR;
      }
      if (TFS_ERROR == client->connect())
      {
        CLIENT_POOL.release_client(client);
        TBSYS_LOG(ERROR, "connect to %s fail, blockid: %u", tbsys::CNetUtil::addrToString(ds_ip).c_str(), block_id);
        return TFS_ERROR;
      }

      //replicate block file
      int32_t len = 0, offset = 0;
      int ret = TFS_SUCCESS;
      char tmp_data_buf[MAX_READ_SIZE];

      //this block will not be write or update now, locked by ns
      int32_t total_len = logic_block->get_data_file_size();
      while (offset < total_len || 0 == total_len)
      {
        int32_t read_len = MAX_READ_SIZE;
        ret = logic_block->read_raw_data(tmp_data_buf, read_len, offset);
        if (TFS_SUCCESS != ret)
        {
          TBSYS_LOG(ERROR, "read raw data fail, ip: %s, blockid: %u, offset: %d, reading len: %d, ret: %d",
              tbsys::CNetUtil::addrToString(ds_ip).c_str(), block_id, offset, read_len, ret);
          CLIENT_POOL.release_client(client);
          return TFS_ERROR;
        }
        len = read_len;

        TBSYS_LOG(DEBUG, "replicate read raw data blockid: %u, offset: %d, read len: %d, total len: %d\n", block_id,
            offset, len, total_len);

        WriteRawDataMessage req_wrd_msg;
        req_wrd_msg.set_block_id(block_id);
        req_wrd_msg.set_offset(offset);
        req_wrd_msg.set_length(len);
        req_wrd_msg.set_data(tmp_data_buf);

        //new block
        if (0 == offset)
        {
          req_wrd_msg.set_new_block(1);
        }

        Message* rsp_msg = client->call(&req_wrd_msg);
        if (NULL != rsp_msg)
        {
          if (rsp_msg->get_message_type() == STATUS_MESSAGE)
          {
            StatusMessage* sm = dynamic_cast<StatusMessage*> (rsp_msg);
            if (STATUS_MESSAGE_OK != sm->get_status())
            {
              TBSYS_LOG(ERROR, "write raw data to %s fail, blockid: %u, offset:%d, reading len: %d",
                  tbsys::CNetUtil::addrToString(ds_ip).c_str(), block_id, offset, len);
              ret = TFS_ERROR;
            }
          }
          tbsys::gDelete(rsp_msg);
        }
        else
        {
          TBSYS_LOG(ERROR, "write raw data to %s fail, blockid: %u, offset: %u, reading len: %d",
              tbsys::CNetUtil::addrToString(ds_ip).c_str(), block_id, offset, len);
          ret = TFS_ERROR;
        }
        if (TFS_SUCCESS != ret)
        {
          break;
        }

        offset += len;
        if (len != MAX_READ_SIZE)
        {
          break;
        }
      }

      if (TFS_SUCCESS != ret)
      {
        CLIENT_POOL.release_client(client);
        return TFS_ERROR;
      }

      //use read index file direct maybe faster
      RawMetaVec raw_meta_vec;
      ret = logic_block->get_meta_infos(raw_meta_vec);
      if (TFS_SUCCESS != ret)
      {
        TBSYS_LOG(ERROR, "replicate get meta info fail, blockid: %u, ret: %d",
            tbsys::CNetUtil::addrToString(ds_ip).c_str(), block_id, ret);
        CLIENT_POOL.release_client(client);
        return TFS_ERROR;
      }

      WriteInfoBatchMessage req_wib_msg;
      req_wib_msg.set_block_id(block_id);
      req_wib_msg.set_offset(0);
      req_wib_msg.set_length(raw_meta_vec.size());
      req_wib_msg.set_raw_meta_list(&raw_meta_vec);
      req_wib_msg.set_block_info(logic_block->get_block_info());
      if (COPY_BETWEEN_CLUSTER == b->server_count_)
      {
        req_wib_msg.set_cluster(COPY_BETWEEN_CLUSTER);
      }

      TBSYS_LOG(DEBUG, "replicate get meta info. blockid: %u, meta info size: %u, cluster flag: %d\n", block_id,
          raw_meta_vec.size(), req_wib_msg.get_cluster());

      ret = TFS_SUCCESS;
      Message* rsp_msg = client->call(&req_wib_msg);
      if (rsp_msg != NULL)
      {
        if (STATUS_MESSAGE == rsp_msg->get_message_type())
        {
          StatusMessage* sm = dynamic_cast<StatusMessage*> (rsp_msg);
          if (STATUS_MESSAGE_OK != sm->get_status())
          {
            TBSYS_LOG(ERROR, "write meta info to %s fail, blockid: %u", tbsys::CNetUtil::addrToString(ds_ip).c_str(),
                block_id);
            ret = TFS_ERROR;
          }
        }
        tbsys::gDelete(rsp_msg);
      }
      else
      {
        TBSYS_LOG(ERROR, "write meta info to %s fail, blockid: %u", tbsys::CNetUtil::addrToString(ds_ip).c_str(),
            block_id);
        ret = TFS_ERROR;
      }

      CLIENT_POOL.release_client(client);
      return ret;
    }

    int ReplicateBlock::add_repl_task(ReplBlock* tmp_rep_blk)
    {
      int repl_exist = 0;
      repl_block_monitor_.lock();
      if (replicating_block_map_.find(tmp_rep_blk->block_id_) != replicating_block_map_.end())
      {
        repl_exist = 1;
      }
      else
      {
        for (uint32_t i = 0; i < repl_block_queue_.size(); ++i)
        {
          if (repl_block_queue_[i]->block_id_ == tmp_rep_blk->block_id_)
          {
            repl_exist = 1;
            break;
          }
        }
      }

      TBSYS_LOG(DEBUG, "add repl task. blockid: %u, is exist: %d\n", tmp_rep_blk->block_id_, repl_exist);

      if (0 == repl_exist)
      {
        tmp_rep_blk->start_time_ = time(NULL);
        repl_block_queue_.push_back(tmp_rep_blk);
      }
      else
      {
        tbsys::gDelete(tmp_rep_blk);
      }
      repl_block_monitor_.unlock();
      repl_block_monitor_.lock();
      repl_block_monitor_.notify();
      repl_block_monitor_.unlock();

      return repl_exist;
    }

    int ReplicateBlock::add_cloned_block_map(const uint32_t block_id)
    {
      ClonedBlock* cloned_block = new ClonedBlock();

      cloned_block->blockid_ = block_id;
      cloned_block->start_time_ = time(NULL);
      cloned_block_mutex_.lock();
      cloned_block_map_.insert(ClonedBlockMap::value_type(block_id, cloned_block));
      cloned_block_mutex_.unlock();

      return TFS_SUCCESS;
    }

    int ReplicateBlock::del_cloned_block_map(const uint32_t block_id)
    {
      TBSYS_LOG(DEBUG, "del cloned block map blockid :%u", block_id);
      cloned_block_mutex_.lock();
      ClonedBlockMapIter mit = cloned_block_map_.find(block_id);
      if (mit != cloned_block_map_.end())
      {
        tbsys::gDelete(mit->second);
        cloned_block_map_.erase(mit);
      }
      cloned_block_mutex_.unlock();

      return TFS_SUCCESS;
    }

    int ReplicateBlock::clear_cloned_block_map()
    {
      int ret = TFS_SUCCESS;
      cloned_block_mutex_.lock();
      for (ClonedBlockMapIter mit = cloned_block_map_.begin(); mit != cloned_block_map_.end(); ++mit)
      {
        ret = BlockFileManager::get_instance()->del_block(mit->first);
        if (TFS_SUCCESS != ret)
        {
          TBSYS_LOG(ERROR, "in check thread: del blockid: %u error. ret: %d", mit->first, ret);
        }

        tbsys::gDelete(mit->second);
      }
      cloned_block_map_.clear();
      cloned_block_mutex_.unlock();

      return TFS_SUCCESS;
    }

    int ReplicateBlock::expire_cloned_block_map()
    {
      int ret = TFS_SUCCESS;
      int32_t current_time = time(NULL);
      int32_t now_time = current_time - expire_cloned_interval_;
      if (last_expire_cloned_block_time_ < now_time)
      {
        cloned_block_mutex_.lock();
        int old_cloned_block_size = cloned_block_map_.size();
        for (ClonedBlockMapIter mit = cloned_block_map_.begin(); mit != cloned_block_map_.end();)
        {
          if (now_time < 0)
            break;
          if (mit->second->start_time_ < now_time)
          {
            ret = BlockFileManager::get_instance()->del_block(mit->first);
            if (TFS_SUCCESS != ret)
            {
              TBSYS_LOG(ERROR, "in check thread: del blockid: %u error. ret: %d", mit->first, ret);
            }

            tbsys::gDelete(mit->second);
            cloned_block_map_.erase(mit++);
          }
          else
          {
            ++mit;
          }
        }

        int32_t new_cloned_block_size = cloned_block_map_.size();
        last_expire_cloned_block_time_ = current_time;
        cloned_block_mutex_.unlock();
        TBSYS_LOG(INFO, "cloned block map: old: %d, new: %d", old_cloned_block_size, new_cloned_block_size);
      }
      return TFS_SUCCESS;
    }

  }
}
