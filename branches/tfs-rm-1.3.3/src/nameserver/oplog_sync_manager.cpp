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
#include <tbsys.h>
#include <Memory.hpp>
#include <atomic.h>
#include "ns_define.h"
#include "layout_manager.h"
#include "global_factory.h"
#include "common/error_msg.h"
#include "common/config.h"
#include "common/config_item.h"
#include "oplog_sync_manager.h"
#include "message/client_manager.h"

using namespace tfs::common;
using namespace tfs::message;
using namespace tbsys;
namespace tfs
{
  namespace nameserver
  {

    FlushOpLogTimerTask::FlushOpLogTimerTask(LayoutManager& mm) :
      meta_mgr_(mm)
    {

    }

    void FlushOpLogTimerTask::runTimerTask()
    {
      NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
      OpLogSyncManager* opLogSync = meta_mgr_.get_oplog_sync_mgr();
      if ((ngi.owner_role_ != NS_ROLE_MASTER) || (opLogSync->is_destroy_))
      {
        TBSYS_LOG(DEBUG, "%s", " wait for flush oplog....");
        return;
      }

      if (ngi.other_side_status_ != NS_STATUS_INITIALIZED || ngi.sync_oplog_flag_ == NS_SYNC_DATA_FLAG_NO)
      {
        //wait
        TBSYS_LOG(DEBUG, "%s", " wait for flush oplog....");
        return;
      }
      const int iret = opLogSync->flush_oplog();
      if (iret != TFS_SUCCESS)
      {
        TBSYS_LOG(ERROR, "flush oplog to filequeue failed(%d)...", iret);
      }
    }

    OpLogSyncManager::OpLogSyncManager(LayoutManager& mm) :
      is_destroy_(false), meta_mgr_(mm), oplog_(NULL), file_queue_(NULL), file_queue_thread_(NULL)
    {

    }

    OpLogSyncManager::~OpLogSyncManager()
    {
      tbsys::gDelete( file_queue_);
      tbsys::gDelete( file_queue_thread_);
      tbsys::gDelete( oplog_);
    }

    int OpLogSyncManager::initialize()
    {
      int iret = TFS_SUCCESS;
      //initializeation filequeue, filequeuethread
      char path[0xff];
      std::string file_queue_name = "oplogsync";
      const char* work_dir = CONFIG.get_string_value(CONFIG_NAMESERVER, CONF_WORK_DIR);
      snprintf(path, 0xff, "%s/filequeue", work_dir);
      std::string file_path(path);
      ARG_NEW(file_queue_, FileQueue, file_path, file_queue_name, FILE_QUEUE_MAX_FILE_SIZE);
      ARG_NEW(file_queue_thread_, FileQueueThread, file_queue_, this);

      //initializeation oplog
      int32_t max_slots_size = CONFIG.get_int_value(CONFIG_NAMESERVER, CONF_OPLOG_SYSNC_MAX_SLOTS_NUM);
      if (max_slots_size <= 0)
        max_slots_size = 1024;//40KB = 40(bytes) * 1024(slots size)
      if (max_slots_size > 4096)
        max_slots_size = 0x03;//100KB
      std::string queue_header_path = std::string(path) + "/" + file_queue_name;
      ARG_NEW(oplog_, OpLog, queue_header_path, max_slots_size);
      iret = oplog_->initialize();
      if (iret != TFS_SUCCESS)
      {
        TBSYS_LOG(ERROR, "%s", "initialization oplog fail...");
        return iret;
      }

      //initializeation fsimage
      std::string tmp(path);
      iret = replay_all();
      if (iret != TFS_SUCCESS)
      {
        TBSYS_LOG(ERROR, "replay all logs error(%d)", iret);
        return iret;
      }

      //reset filequeue
      file_queue_->set_delete_file_flag(false);
      file_queue_->clear();
      const QueueInformationHeader* qhead = file_queue_->get_queue_information_header();
      OpLogRotateHeader rotmp;
      rotmp.rotate_seqno_ = qhead->write_seqno_;
      rotmp.rotate_offset_ = qhead->write_filesize_;

      //update rotate header information
      oplog_->update_oplog_rotate_header(rotmp);

      // add flush oplog timer
      FlushOpLogTimerTaskPtr foltt = new FlushOpLogTimerTask(meta_mgr_);
      GFactory::get_timer()->scheduleRepeated(foltt, tbutil::Time::seconds(
          SYSPARAM_NAMESERVER.heart_interval_));

      file_queue_thread_->initialize(1, OpLogSyncManager::do_sync_oplog);
      const int queue_thread_num = CONFIG.get_int_value(CONFIG_NAMESERVER, CONF_OPLOGSYNC_THREAD_NUM, 1);
      work_thread_.setThreadParameter(queue_thread_num , this, NULL);
      work_thread_.start();
      TBSYS_LOG(DEBUG, "%s", "start ns's log sync");
      return TFS_SUCCESS;
    }

    int OpLogSyncManager::wait_for_shut_down()
    {
      if (file_queue_thread_ != NULL)
      {
        file_queue_thread_->wait();
      }
      work_thread_.wait();
      return TFS_SUCCESS;
    }

    int OpLogSyncManager::destroy()
    {
      tbutil::Monitor<tbutil::Mutex>::Lock lock(monitor_);
      monitor_.notifyAll();
      is_destroy_ = true;
      if (file_queue_thread_ != NULL)
      {
        file_queue_thread_->destroy();
      }
      work_thread_.stop();
      return TFS_SUCCESS;
    }

    int OpLogSyncManager::register_slots(const char* const data, const int64_t length)
    {
      NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
      if ((ngi.owner_role_ != NS_ROLE_MASTER) || (is_destroy_))
      {
        return EXIT_REGISTER_OPLOG_SYNC_ERROR;
      }
      if (data == NULL || length <= 0)
      {
        return TFS_SUCCESS;
      }
      file_queue_thread_->write(data, length);
      return TFS_SUCCESS;
    }

    int OpLogSyncManager::register_msg(const Message* msg)
    {
      NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
      if ((ngi.owner_role_ != NS_ROLE_MASTER) || (ngi.other_side_status_ < NS_STATUS_ACCEPT_DS_INFO)//slave dead or uninitialize
          || (ngi.sync_oplog_flag_ < NS_SYNC_DATA_FLAG_READY) || (is_destroy_))
      {
        return EXIT_REGISTER_OPLOG_SYNC_ERROR;
      }

      Message* smsg = NULL;//NewClientManager::get_instance().get_msg_factory().clone_message(const_cast<Message*> (msg), 2, true);
      if (smsg != NULL)
        push(smsg);
      return TFS_SUCCESS;
    }

    void OpLogSyncManager::notify_all()
    {
      tbutil::Monitor<tbutil::Mutex>::Lock lock(monitor_);
      monitor_.notifyAll();
    }

    void OpLogSyncManager::rotate()
    {
      const QueueInformationHeader* head = file_queue_->get_queue_information_header();
      OpLogRotateHeader ophead;
      ophead.rotate_seqno_ = head->write_seqno_;
      ophead.rotate_offset_ = head->write_filesize_;
      const OpLogRotateHeader* tmp = oplog_->get_oplog_rotate_header();
      oplog_->update_oplog_rotate_header(ophead);
      TBSYS_LOG(
          INFO,
          "queue header: readseqno(%d), read_offset(%d), write_seqno(%d),\
                        write_file_size(%d), queuesize(%d). oplogheader:, rotate_seqno(%d)\
                        rotate_offset(%d), last_rotate_seqno(%d), last_rotate_offset(%d)",
          head->read_seqno_, head->read_offset_, head->write_seqno_, head->write_filesize_, head->queue_size_,
          ophead.rotate_seqno_, ophead.rotate_offset_, tmp->rotate_seqno_, tmp->rotate_offset_);
    }

    //only debug
    std::string OpLogSyncManager::printDsList(const VUINT64& ds_list)
    {
      std::string ipstr;
      size_t iSize = ds_list.size();
      for (size_t i = 0; i < iSize; ++i)
      {
        ipstr += tbsys::CNetUtil::addrToString(ds_list[i]);
        ipstr += "/";
      }
      return ipstr;
    }

    int OpLogSyncManager::flush_oplog()
    {
      tbutil::Mutex::Lock lock(mutex_);
      time_t iNow = time(NULL);
      if (oplog_->finish(iNow, true))
      {
        register_slots(oplog_->get_buffer(), oplog_->get_slots_offset());
        TBSYS_LOG(DEBUG, "oplog size(%d)", oplog_->get_slots_offset());
        oplog_->reset(iNow);
      }
      return TFS_SUCCESS;
    }

    int OpLogSyncManager::log(uint8_t type, const char* const data, const int64_t length)
    {
      int iret = TFS_SUCCESS;
      NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
      if (ngi.owner_role_ != NS_ROLE_MASTER)
      {
        return TFS_SUCCESS;
      }
      #if !defined(TFS_NS_GTEST) && !defined(TFS_NS_INTEGRATION)
      int count = 0;
      tbutil::Mutex::Lock lock(mutex_);
      do
      {
        ++count;
        iret = oplog_->write(type, data, length);
        if (iret != TFS_SUCCESS)
        {
          if (iret == EXIT_SLOTS_OFFSET_SIZE_ERROR)
          {
            register_slots(oplog_->get_buffer(), oplog_->get_slots_offset());
            TBSYS_LOG(DEBUG, "oplog size(%d)", oplog_->get_slots_offset());
            oplog_->reset();
          }
        }
      }
      while (count < 0x03 && iret != TFS_SUCCESS);
      if (iret != TFS_SUCCESS)
      {
        TBSYS_LOG(ERROR, "write log file error, type(%d)", type);
        return iret;
      }
      if (oplog_->finish(0))
      {
        register_slots(oplog_->get_buffer(), oplog_->get_slots_offset());
        TBSYS_LOG(DEBUG, "oplog size(%d)", oplog_->get_slots_offset());
        oplog_->reset();
      }
      #endif
      return iret;
    }

    int OpLogSyncManager::push(message::Message* msg, int32_t max_queue_size, bool block)
    {
      return  work_thread_.push(msg, max_queue_size, block);
    }

    int OpLogSyncManager::do_sync_oplog(const void* const data, const int64_t len, const int32_t, void *arg)
    {
      OpLogSyncManager* op_log_sync = static_cast<OpLogSyncManager*> (arg);
      return op_log_sync->do_sync_oplog(static_cast<const char* const > (data), len);
    }

    int OpLogSyncManager::do_sync_oplog(const char* const data, const int64_t length)
    {
      NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
      if ((ngi.owner_role_ != NS_ROLE_MASTER) || (is_destroy_))
      {
        file_queue_->clear();
        TBSYS_LOG(INFO, "%s", " wait for sync oplog");
        tbutil::Monitor<tbutil::Mutex>::Lock lock(monitor_);
        monitor_.wait();
        return TFS_SUCCESS;
      }

      if (ngi.other_side_status_ < NS_STATUS_INITIALIZED || ngi.sync_oplog_flag_ < NS_SYNC_DATA_FLAG_YES)
      {
        //wait
        TBSYS_LOG(INFO, "%s", " wait for sync oplog");
        tbutil::Monitor<tbutil::Mutex>::Lock lock(monitor_);
        monitor_.wait();
      }

      //to send data to the slave & wait
      OpLogSyncMessage msg;
      msg.set_data(data, length);
      Message* rmsg = NULL;
      int32_t count = 0;
      int32_t iret = TFS_ERROR;
      do
      {
        ++count;
        rmsg = NULL;
        NewClient* client = NewClientManager::get_instance().create_client();
        iret = send_msg_to_server(ngi.other_side_ip_port_, client, &msg, rmsg);
        if (TFS_SUCCESS == iret)
        {
          iret = rmsg->getPCode() == OPLOG_SYNC_RESPONSE_MESSAGE ? TFS_SUCCESS : TFS_ERROR;
          if (TFS_SUCCESS == iret)
          {
            OpLogSyncResponeMessage* tmsg = dynamic_cast<OpLogSyncResponeMessage*> (rmsg);
            iret = tmsg->get_complete_flag() == OPLOG_SYNC_MSG_COMPLETE_YES ? TFS_SUCCESS : TFS_ERROR;
            if (TFS_SUCCESS == iret)
            {
              NewClientManager::get_instance().destroy_client(client);
              break;
            }
          }
        }
        NewClientManager::get_instance().destroy_client(client);
      }
      while (count < 0x03);
      if (TFS_ERROR == iret)
      {
        ngi.sync_oplog_flag_ = NS_SYNC_DATA_FLAG_NO;
        TBSYS_LOG(WARN, "synchronization oplog(%s) message failed, count(%d)", data, count);
      }
      return TFS_SUCCESS;
    }

    bool OpLogSyncManager::handlePacketQueue(tbnet::Packet *packet, void *args) 
    {
      bool bret = packet != NULL;
      if (bret)
      {
        Message* message = dynamic_cast<Message*>(packet);
        NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
        int iret = TFS_SUCCESS;
        if (ngi.owner_role_ == NS_ROLE_MASTER) //master
          iret = do_master_msg(message, args);
        else if (ngi.owner_role_ == NS_ROLE_SLAVE) //slave
          iret = do_slave_msg(message, args);
        tbsys::gDelete(message);
        return iret;
      }
      return TFS_ERROR;
    }

    int OpLogSyncManager::do_sync_oplog(const Message* message, const void*)
    {
      if (is_destroy_)
        return TFS_SUCCESS;

      const OpLogSyncMessage* msg = dynamic_cast<const OpLogSyncMessage*> (message);
      const char* data = msg->get_data();
      int64_t length = msg->get_length();
      int64_t offset = 0;
      time_t now = time(NULL);
      int32_t iret = TFS_SUCCESS;

      while ((offset < length) 
            && (GFactory::get_runtime_info().destroy_flag_!= NS_DESTROY_FLAGS_YES))
      {
        iret = replay_helper(data, length, offset, now);
        if ((iret != TFS_SUCCESS)
             && (iret != EXIT_PLAY_LOG_ERROR))
        {
          break;
        }
      }
      OpLogSyncResponeMessage* rmsg = NULL;
      ARG_NEW(rmsg, OpLogSyncResponeMessage);
      rmsg->set_complete_flag();
      const_cast<Message*> (message)->reply_message(rmsg);
      return TFS_SUCCESS;
    }

    int OpLogSyncManager::do_master_msg(const Message* msg, const void*)
    {
      if (is_destroy_)
      {
        return TFS_SUCCESS;
      }
      NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
      if (ngi.other_side_status_ < NS_STATUS_INITIALIZED || ngi.sync_oplog_flag_ < NS_SYNC_DATA_FLAG_YES)
      {
        //wait
        TBSYS_LOG(DEBUG, "%s", "wait for sync message");
        tbutil::Monitor<tbutil::Mutex>::Lock lock(monitor_);
        monitor_.wait();
      }
      int32_t iret = TFS_ERROR;
      int32_t count(0);
      do
      {
        ++count;
        iret = send_msg_to_server(ngi.other_side_ip_port_, const_cast<Message*>(msg));
        if (STATUS_MESSAGE_OK == iret)
        {
          iret = TFS_SUCCESS;
          break;
        }
      }
      while (count < 0x03);
      if (TFS_ERROR == iret)
      {
        ngi.sync_oplog_flag_ = NS_SYNC_DATA_FLAG_NO;
        TBSYS_LOG(WARN, "synchronization operation(%s) message failed, count(%d)", NULL, count);
      }
      return TFS_SUCCESS;
    }

    int OpLogSyncManager::do_slave_msg(const Message* msg, const void* args)
    {
      return do_sync_oplog(msg, args);
    }

    int OpLogSyncManager::replay_helper(const char* const data, int64_t& length, int64_t& offset, time_t now)
    {
      bool bret = (data != NULL && length > static_cast<int64_t>(sizeof(OpLogHeader)));
      if (bret)
      {
        OpLogHeader* header = (OpLogHeader*)(data + offset); 
        offset += sizeof(OpLogHeader);
        uint32_t crc = 0;
        crc = Func::crc(crc, header->data_, header->length_);
        if (crc != header->crc_)
        {
          TBSYS_LOG(ERROR, "check crc(%u)<>(%u) error", header->crc_, crc);
          return EXIT_CHECK_CRC_ERROR;
        }
        int8_t type = header->type_;
        switch (type)
        {
        case OPLOG_TYPE_REPLICATE_MSG:
        {
          ReplicateBlockMessage* msg = NULL;
          ARG_NEW(msg, ReplicateBlockMessage);
          if (msg->deserialize(data, length, offset) < 0)
          {
            tbsys::gDelete(msg);
            TBSYS_LOG(ERROR, "deserialize error, data(%s), length(%"PRI64_PREFIX"d) offset(%"PRI64_PREFIX"d)", data, length, offset);
            return EXIT_DESERIALIZE_ERROR;
          }
          //push msg to work queue
          tbsys::gDelete(msg);
        }
        break;
        case OPLOG_TYPE_COMPACT_MSG:
        {
          CompactBlockCompleteMessage* msg = NULL;
          ARG_NEW(msg, CompactBlockCompleteMessage);
          if (msg->deserialize(data, length, offset) < 0)
          {
            tbsys::gDelete(msg);
            TBSYS_LOG(ERROR, "deserialize error, data(%s), length(%"PRI64_PREFIX"d) offset(%"PRI64_PREFIX"d)", data, length, offset);
            return EXIT_DESERIALIZE_ERROR;
          }
          //push_msg to work queue
          tbsys::gDelete(msg);
        }
        break;
        case OPLOG_TYPE_BLOCK_OP:
        {
          BlockOpLog oplog;
          memset(&oplog, 0, sizeof(oplog));
          if (oplog.deserialize(data, length, offset) < 0)
          {
            TBSYS_LOG(ERROR, "deserialize error, data(%s), length(%"PRI64_PREFIX"d) offset(%"PRI64_PREFIX"d)", data, length, offset);
            return EXIT_DESERIALIZE_ERROR;
          }
          
          if ((oplog.servers_.empty())
              || (oplog.blocks_.empty()))
          {
            TBSYS_LOG(ERROR, "play log error, data(%s), length(%"PRI64_PREFIX"d) offset(%"PRI64_PREFIX"d)", data, length, offset);
            return EXIT_PLAY_LOG_ERROR;
          }
            
          int8_t cmd = oplog.cmd_;
          std::vector<uint32_t>::iterator iter = oplog.blocks_.begin();
          for (; iter != oplog.blocks_.end(); ++iter)
          {
            switch (cmd)
            {
            case OPLOG_UPDATE:
            {
              bool addnew = false;
              if (meta_mgr_.update_block_info(oplog.info_, oplog.servers_[0], now, addnew) != TFS_SUCCESS)
              {
                TBSYS_LOG(WARN, "update block information error, block(%u), server(%s)",
                  oplog.info_.block_id_, CNetUtil::addrToString(oplog.servers_[0]).c_str());
              }
            }
            break;
            case OPLOG_INSERT:
            {
              uint32_t block_id = (*iter);
              BlockChunkPtr ptr = meta_mgr_.get_chunk(block_id);
              BlockCollect* block = NULL;
              {
                RWLock::Lock lock(*ptr, WRITE_LOCKER);
                block = ptr->find(block_id);
                if (block == NULL)
                {
                  block = meta_mgr_.add_block(block_id);
                  if (block == NULL)
                  {
                    TBSYS_LOG(WARN, "add block(%u) error", block_id);
                    return EXIT_PLAY_LOG_ERROR;
                  }
                }
                block->update(oplog.info_);
              }

              ServerCollect* server = NULL;
              std::vector<uint64_t>::iterator s_iter = oplog.servers_.begin();
              for (; s_iter != oplog.servers_.end(); ++s_iter)
              {
                {
                  server = meta_mgr_.get_server((*s_iter));
                }
                if (server == NULL)
                {
                  TBSYS_LOG(WARN, "server object not found by (%s)", CNetUtil::addrToString((*s_iter)).c_str());
                  continue;
                }
                RWLock::Lock lock(*ptr, WRITE_LOCKER);
                block = ptr->find(block_id);
                if (meta_mgr_.build_relation(block, server, now) != TFS_SUCCESS)
                {
                  TBSYS_LOG(WARN, "build relation between block(%u) and server(%s) failed",
                    block_id, CNetUtil::addrToString((*s_iter)).c_str());
                }
              }
            }
            break;
            case OPLOG_REMOVE:
            {
              uint32_t block_id = (*iter);
              BlockChunkPtr ptr = meta_mgr_.get_chunk(block_id);
              RWLock::Lock lock(*ptr, WRITE_LOCKER);
              ptr->remove(block_id);
            }
            break;
            case OPLOG_RELIEVE_RELATION:
            {
              uint32_t block_id = (*iter);
              BlockChunkPtr ptr = meta_mgr_.get_chunk(block_id);
              BlockCollect* block = NULL;
              ServerCollect* server = NULL;
              std::vector<uint64_t>::iterator s_iter = oplog.servers_.begin();
              for (; s_iter != oplog.servers_.end(); ++s_iter)
              {
                {
                  server = meta_mgr_.get_server((*s_iter));
                }
                if (server == NULL)
                {
                  TBSYS_LOG(WARN, "server object not found by (%s)", CNetUtil::addrToString((*s_iter)).c_str());
                  continue;
                }

                RWLock::Lock lock(*ptr, WRITE_LOCKER);
                block = ptr->find(block_id);
                if (meta_mgr_.relieve_relation(block, server, now))
                {
                  TBSYS_LOG(WARN, "relieve relation between block(%u) and server(%s) failed",
                    block_id, CNetUtil::addrToString((*s_iter)).c_str());
                }
              }
            }
            break;
            default:
              TBSYS_LOG(WARN, "type(%d) and  cmd(%d) not found", type, cmd);
              return EXIT_PLAY_LOG_ERROR;
            };
          }
        }
        break;
        default:
          TBSYS_LOG(WARN, "type(%d) not found", type);
          return EXIT_PLAY_LOG_ERROR;
        }
        return TFS_SUCCESS;
      }
      return TFS_ERROR;
    }

    int OpLogSyncManager::replay_all()
    {
      bool has_log = false;
      file_queue_->set_delete_file_flag(true);
      int iret = file_queue_->load_queue_head();
      if (iret != TFS_SUCCESS)
      {
        TBSYS_LOG(DEBUG, "load header file of file_queue errors(%s)", strerror(errno));
        return iret;
      }
      OpLogRotateHeader head = *oplog_->get_oplog_rotate_header();
      QueueInformationHeader* qhead = file_queue_->get_queue_information_header();
      QueueInformationHeader tmp = *qhead;
      TBSYS_LOG(DEBUG, "befor load queue header: read seqno(%d), read offset(%d), write seqno(%d),"
        "write file size(%d), queue size(%d). oplog header:, rotate seqno(%d)"
        "rotate offset(%d)", qhead->read_seqno_, qhead->read_offset_, qhead->write_seqno_, qhead->write_filesize_,
          qhead->queue_size_, head.rotate_seqno_, head.rotate_offset_);
      if (qhead->read_seqno_ > 0x01 && qhead->write_seqno_ > 0x01 && head.rotate_seqno_ > 0)
      {
        has_log = true;
        if (tmp.read_seqno_ <= head.rotate_seqno_)
        {
          tmp.read_seqno_ = head.rotate_seqno_;
          if (tmp.read_seqno_ == head.rotate_seqno_)
            tmp.read_offset_ = head.rotate_offset_;
          file_queue_->update_queue_information_header(&tmp);
        }
      }
      TBSYS_LOG(DEBUG, "after load queue header: read seqno(%d), read offset(%d), write seqno(%d),"
        "write file size(%d), queue size(%d). oplog header:, rotate seqno(%d)"
        "rotate offset(%d)", qhead->read_seqno_, qhead->read_offset_, qhead->write_seqno_, qhead->write_filesize_,
          qhead->queue_size_, head.rotate_seqno_, head.rotate_offset_);

      iret = file_queue_->initialize();
      if (iret != TFS_SUCCESS)
      {
        TBSYS_LOG(DEBUG, "call FileQueue::finishSetup errors(%s)", strerror(errno));
        return iret;
      }

      if (has_log)
      {
        time_t now = time(NULL);
        int64_t length = 0;
        int64_t offset = 0;
        int32_t iret = TFS_SUCCESS;
        do
        {
          QueueItem* item = file_queue_->pop();
          if (item == NULL)
          {
            continue;
          }
          const char* const data = item->data_;
          length = item->length_;
          offset = 0;
          do
          {
            iret = replay_helper(data, length, offset, now);
            if ((iret != TFS_SUCCESS)
                && (iret != EXIT_PLAY_LOG_ERROR))
            {
              break;
            }
          }
          while ((length > static_cast<int64_t> (sizeof(OpLogHeader)))
                  && (length > offset)
                  && (GFactory::get_runtime_info().destroy_flag_!= NS_DESTROY_FLAGS_YES));
          free(item);
          item = NULL;
        }
        while ((qhead->read_seqno_ != qhead->write_seqno_) || ((qhead->read_seqno_ == qhead->write_seqno_)
            && (qhead->read_offset_ != qhead->write_filesize_)) && (GFactory::get_runtime_info().destroy_flag_ != NS_DESTROY_FLAGS_YES));
      }
      return TFS_SUCCESS;
    }
  }//end namespace nameserver
}//end namespace tfs
