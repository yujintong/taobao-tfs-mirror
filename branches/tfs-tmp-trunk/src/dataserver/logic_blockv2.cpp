/*
 * (C) 2007-2013 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id
 *
 * Authors:
 *   duanfei <duanfei@taobao.com>
 *      - initial release
 *
 */

#include "common/error_msg.h"

#include "ds_define.h"
#include "logic_blockv2.h"
#include "super_block_manager.h"
#include "physical_block_manager.h"

using namespace tfs::common;

namespace tfs
{
  namespace dataserver
  {
    LogicBlock::LogicBlock(const uint64_t logic_block_id, const std::string& index_path):
      logic_block_id_(logic_block_id),
      data_handle_(*this),
      index_handle_(new IndexHandle(index_path))
    {

    }

    LogicBlock::~LogicBlock()
    {
      tbsys::gDelete(index_handle_);
    }

    int LogicBlock::create_index(const int32_t bucket_size, const common::MMapOption mmap_option)
    {
      int32_t ret = (bucket_size > 0 && mmap_option.check()) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        ret = index_handle_->create(id(), bucket_size, mmap_option);
      }
      return ret;
    }

    int LogicBlock::load_index(const common::MMapOption mmap_option)
    {
      int32_t ret = (mmap_option.check()) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        ret = index_handle_->mmap(id(), mmap_option);
      }
      return ret;
    }

    int LogicBlock::remove_self_file()
    {
      int32_t ret = index_handle_->remove_self(id());
      if (TFS_SUCCESS == ret)
      {
        //TODO;
        //SuperBlockManager supber_block_manager;
        //SuperBlockManager& supber_block_manager = BlockManager::instance().get_super_block_manager();
        PHYSICAL_BLOCK_LIST_CONST_ITER iter = physical_block_list_.begin();
        for (; iter != physical_block_list_.end() && TFS_SUCCESS == ret; ++iter)
        {
          //ret = supber_block_manager.cleanup_block_index((*iter)->id());
        }
      }
      return ret;
    }

    int LogicBlock::rename_index_filename()
    {
      return index_handle_->rename_filename(id());
    }

    int LogicBlock::generation_file_id(uint64_t& fileid, const double threshold)
    {
      return index_handle_->generation_file_id(fileid, threshold);
    }

    int LogicBlock::add_physical_block(PhysicalBlock* physical_block)
    {
      int32_t ret = (NULL != physical_block) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        physical_block_list_.push_back(physical_block);
      }
      return ret;
    }

    int LogicBlock::choose_physic_block(PhysicalBlock*& block, int32_t& length, int32_t& inner_offset, const int32_t offset) const
    {
      int32_t ret = (offset >= 0 && length >= 0) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        bool result = (length == 0);
        int32_t total_length = 0;
        PHYSICAL_BLOCK_LIST_CONST_ITER iter = physical_block_list_.begin();
        for (; iter != physical_block_list_.end() && !result; ++iter)
        {
          total_length += (*iter)->length();
          result = (offset < total_length);
          if (result)
          {
            block = (*iter);
            inner_offset = total_length - offset;
            length       = (*iter)->length() - inner_offset;
          }
        }
        ret = (result) ? TFS_SUCCESS : EXIT_PHYSIC_BLOCK_OFFSET_ERROR;
      }
      return ret;
    }

    int LogicBlock::check_block_version(common::BlockInfoV2& info, const int32_t remote_version, const int8_t index, const uint64_t logic_block_id)
    {
      int32_t ret = (INVALID_BLOCK_ID != logic_block_id) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        ret = index_handle_->check_block_version(info, remote_version, index);
      }
      return ret;
    }

    int LogicBlock::check_block_intact()
    {
      int32_t ret = TFS_SUCCESS;
      return ret;
    }

    int LogicBlock::update_block_info(const BlockInfoV2& info, const uint64_t logic_block_id) const
    {
      return index_handle_->update_block_info(info, logic_block_id);
    }

    int LogicBlock::update_block_version(const int8_t step, const uint64_t logic_block_id)
    {
      int32_t ret = (INVALID_BLOCK_ID != logic_block_id && step >= 0) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        ret = index_handle_->update_block_version(step, logic_block_id);
      }
      return ret;
    }

    int LogicBlock::write(const uint64_t fileid, DataFile& datafile, const uint32_t crc, const uint64_t logic_block_id)
    {
      UNUSED(logic_block_id);
      int32_t ret = (INVALID_FILE_ID != fileid) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        std::string path;
        SuperBlockInfo sbinfo;
        SuperBlockManager supber_block_manager(path);//TODO
        ret = supber_block_manager.get_super_block_info(sbinfo);
        if (TFS_SUCCESS == ret)
        {
          FileInfoV2 finfo;
          ret = index_handle_->read_file_info(finfo, fileid, sbinfo.max_use_hash_bucket_ratio_, id());
          bool update = (TFS_SUCCESS == ret);
          if (update)
              TBSYS_LOG(INFO, "file exist, update! block id: %"PRI64_PREFIX"u, fileid: %"PRI64_PREFIX"d", id(), fileid );

          time_t now = time(NULL);
          int32_t file_size = datafile.length();
          FileInfoV2 new_finfo;
          new_finfo.id_ = fileid;
          new_finfo.create_time_ = update ? finfo.create_time_ : now;
          new_finfo.modify_time_ = now;
          new_finfo.crc_ = crc;
          new_finfo.status_ = FILE_STATUS_NOMARL;
          new_finfo.next_ = 0;
          new_finfo.size_ = file_size;
          ret = index_handle_->get_used_offset(new_finfo.offset_, id());
          if (TFS_SUCCESS == ret)
          {
            ret = extend_block_(new_finfo.size_, new_finfo.offset_);
            if (TFS_SUCCESS == ret)//write data
            {
              char* data = NULL;
              int32_t offset = 0;
              while (offset < new_finfo.size_  && TFS_SUCCESS == ret)
              {
                int32_t length = new_finfo.size_ - offset;
                ret = datafile.pread(data, length, offset);
                ret = ret >= 0 ? TFS_SUCCESS : ret;
                if (TFS_SUCCESS == ret)
                {
                  assert(NULL != data);
                  offset += length;
                  ret = offset > new_finfo.size_  ? EXIT_WRITE_OFFSET_ERROR : TFS_SUCCESS;
                  if (TFS_SUCCESS == ret && length > 0)
                  {
                    int32_t write_offset = 0;
                    while (write_offset < length && TFS_SUCCESS == ret)
                    {
                      int32_t write_length = length - write_offset;
                      ret = data_handle_.pwrite((data + write_offset), write_length, (new_finfo.offset_ + write_offset));
                      ret = ret >= 0 ? TFS_SUCCESS : ret;
                      if (TFS_SUCCESS == ret)
                      {
                        write_offset += write_length;
                      }
                    }//end while (write_offset < length && TFS_SUCCESS == ret)
                  }
                }
              }//end while (offset < new_finfo.size_  && TFS_SUCCESS == ret)
              //这里并没有直接将数据刷到磁盘，如果掉电的话，有可能丢数据, 我们需要多个副本来保证数据的正确性
            }

            if (TFS_SUCCESS == ret)
            {
              ret = index_handle_->update_block_info(update ? OPER_UPDATE : OPER_INSERT, new_finfo.size_, finfo.size_, false, id());
              if (TFS_SUCCESS == ret)
              {
                ret = index_handle_->write_file_info(new_finfo, fileid, sbinfo.max_use_hash_bucket_ratio_, update, id());
                if (TFS_SUCCESS != ret)
                {
                  TBSYS_LOG(INFO, "write file info failed, we'll rollback, ret: %d, block id: %"PRI64_PREFIX"u, fileid:%"PRI64_PREFIX"u",
                      ret, id(), fileid);
                  ret = index_handle_->update_block_info(update ? OPER_UPDATE : OPER_INSERT, new_finfo.size_, finfo.size_, true, id());
                  if (TFS_SUCCESS != ret)
                  {
                    TBSYS_LOG(ERROR, "write file info failed, we'll rollback failed, ret: %d, block id: %"PRI64_PREFIX"u, fileid:%"PRI64_PREFIX"u",
                        ret, id(), fileid);
                  }
                }
                ret = index_handle_->flush();
                if (TFS_SUCCESS != ret)
                {
                  TBSYS_LOG(INFO, "write file, flush index to disk failed, ret: %d, block id: %"PRI64_PREFIX"u, fileid:%"PRI64_PREFIX"u",
                        ret, id(), fileid);
                }
              }
            }
          }
        }
      }
      return TFS_SUCCESS == ret ? datafile.length() : ret;
    }

    int LogicBlock::pwrite(const char* buf, const int32_t nbytes, const int32_t offset, const uint64_t logic_block_id)
    {
      UNUSED(logic_block_id);
      int32_t ret = (NULL != buf && nbytes >= 0) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        ret = extend_block_(nbytes, offset);
        if (TFS_SUCCESS == ret)//write data
        {
          int32_t mem_offset = 0;
          while (nbytes > 0 && mem_offset < nbytes && TFS_SUCCESS == ret)
          {
            int32_t length = nbytes - mem_offset;
            ret = data_handle_.pwrite((buf+ mem_offset), length, (offset + mem_offset));
            ret = ret >= 0 ? TFS_SUCCESS : ret;
            if (TFS_SUCCESS == ret)
            {
              mem_offset += length;
            }
          }
        }
      }
      return TFS_SUCCESS == ret ? nbytes : ret;
    }

    int LogicBlock::write_file_infos(std::vector<FileInfoV2>& infos, const double threshold, const bool override, const uint64_t logic_block_id)
    {
      int32_t ret = (INVALID_BLOCK_ID != logic_block_id) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        ret = index_handle_->write_file_infos(infos, threshold, override, logic_block_id);
      }
      return ret;
    }

    int LogicBlock::read(char* buf, int32_t& nbytes, const int32_t offset, const uint64_t fileid, const int8_t flag, const uint64_t logic_block_id)
    {
      UNUSED(logic_block_id);
      int32_t ret = (NULL != buf && nbytes >= 0 && offset >= 0 && INVALID_FILE_ID != fileid) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        std::string path;
        SuperBlockInfo sbinfo;
        SuperBlockManager supber_block_manager(path);//TODO
        ret = supber_block_manager.get_super_block_info(sbinfo);
        if (TFS_SUCCESS == ret)
        {
          FileInfoV2 finfo;
          ret = index_handle_->read_file_info(finfo, fileid, sbinfo.max_use_hash_bucket_ratio_, id());
          if (TFS_SUCCESS == ret)
          {
            // truncate to right read length
            if (offset + nbytes > finfo.size_)
              nbytes = finfo.size_ - offset;
            ret = (nbytes >= 0) ? TFS_SUCCESS : EXIT_READ_OFFSET_ERROR;
            if (TFS_SUCCESS == ret)
            {
              if (0 == offset)
              {
                if (READ_DATA_OPTION_FLAG_FORCE & flag)
                  ret = (finfo.id_ == fileid && 0 == finfo.status_ & FILE_STATUS_INVALID) ? TFS_SUCCESS : EXIT_FILE_INFO_ERROR;
                else
                  ret = (finfo.id_ == fileid && 0 == finfo.status_ & (FILE_STATUS_DELETE | FILE_STATUS_INVALID | FILE_STATUS_CONCEAL)) ? TFS_SUCCESS : EXIT_FILE_INFO_ERROR;
              }
            }

            if (TFS_SUCCESS == ret && nbytes > 0)
            {
              ret = data_handle_.pread(buf, nbytes, finfo.offset_ + offset);
            }
          }
        }
      }
      return ret;
    }

    int LogicBlock::pread(char* buf, int32_t& nbytes, const int32_t offset,const uint64_t logic_block_id)
    {
      UNUSED(logic_block_id);
      int32_t ret = (NULL != buf && nbytes >= 0 && offset >= 0) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        int32_t inner_offset = 0;
        ret = index_handle_->get_used_offset(inner_offset, logic_block_id);
        if (TFS_SUCCESS == ret)
        {
          if (offset + nbytes > inner_offset)
            nbytes = inner_offset - offset;
          ret = (nbytes >= 0) ? TFS_SUCCESS : EXIT_READ_OFFSET_ERROR;
          if (TFS_SUCCESS == ret && nbytes > 0)
          {
            ret = data_handle_.pread(buf, nbytes, inner_offset + offset);
          }
        }
      }
      return ret;
    }

    int LogicBlock::stat(FileInfoV2& info, const uint64_t fileid, const double threshold, const uint64_t logic_block_id) const
    {
      int32_t ret = (INVALID_FILE_ID != fileid && INVALID_BLOCK_ID != logic_block_id) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        ret = index_handle_->read_file_info(info, fileid, threshold, logic_block_id);
      }
      return ret;
    }

    int LogicBlock::unlink(int64_t& size, const uint64_t fileid, const int32_t action, const uint64_t logic_block_id)
    {
      size = 0;
      UNUSED(logic_block_id);
      int32_t ret = (INVALID_FILE_ID != fileid) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        std::string path;
        SuperBlockInfo sbinfo;
        SuperBlockManager supber_block_manager(path);//TODO
        ret = supber_block_manager.get_super_block_info(sbinfo);
        if (TFS_SUCCESS == ret)
        {
          FileInfoV2 finfo;
          ret = index_handle_->read_file_info(finfo, fileid, sbinfo.max_use_hash_bucket_ratio_, id());
          if (TFS_SUCCESS == ret)
          {
            int32_t oper_type  = OPER_NONE;
            int32_t tmp_action = action;
            int8_t status = FILE_STATUS_NOMARL;
            if (action > REVEAL)//TODO这里其实是有问题，需要修改，统一使用位来操作
            {
              status = (action >> 4) & 0x7;
              tmp_action = SYNC;
            }

            switch(tmp_action)
            {
              case SYNC:
                if ((finfo.status_ & FILE_STATUS_DELETE) != (status & FILE_STATUS_DELETE))
                  oper_type = status & FILE_STATUS_DELETE ? OPER_DELETE : OPER_UNDELETE;
                break;
              case DELETE:
                ret = (0 != (finfo.status_ & (FILE_STATUS_DELETE | FILE_STATUS_INVALID))) ? EXIT_FILE_STATUS_ERROR : TFS_SUCCESS;
                if (TFS_SUCCESS == ret)
                {
                  oper_type = OPER_DELETE;
                  finfo.status_ |= FILE_STATUS_DELETE;
                }
                break;
              case UNDELETE:
                ret = (0 == (finfo.status_ & FILE_STATUS_DELETE)) ? EXIT_FILE_STATUS_ERROR : TFS_SUCCESS;
                if (TFS_SUCCESS == ret)
                {
                  oper_type = OPER_UNDELETE;
                  finfo.status_ &= (~FILE_STATUS_DELETE);
                }
                break;
              case CONCEAL:
                ret = (0 != (finfo.status_ & (FILE_STATUS_DELETE | FILE_STATUS_INVALID | FILE_STATUS_CONCEAL))) ? EXIT_FILE_STATUS_ERROR : TFS_SUCCESS;
                if (TFS_SUCCESS == ret)
                  finfo.status_ |= FILE_STATUS_CONCEAL;
                break;
              case REVEAL:
                ret = ((0 == (finfo.status_ & FILE_STATUS_CONCEAL))
                    || (0 != (finfo.status_ & (FILE_STATUS_DELETE | FILE_STATUS_INVALID)))) ? EXIT_FILE_STATUS_ERROR : TFS_SUCCESS;
                if (TFS_SUCCESS == ret)
                  finfo.status_ &= (~FILE_STATUS_CONCEAL);
                break;
              default:
                ret = EXIT_FILE_ACTION_ERROR;
                TBSYS_LOG(INFO, "action is illegal. action: %d, blockid: %"PRI64_PREFIX"u, fileid: %" PRI64_PREFIX "u, ret: %d",
                  tmp_action, id(), fileid, ret);
                break;
            }
            if (TFS_SUCCESS == ret)
            {
              ret = index_handle_->update_block_info(oper_type, 0, finfo.size_);
              if (TFS_SUCCESS == ret)
              {
                finfo.modify_time_ = time(NULL);
                ret = index_handle_->write_file_info(finfo, fileid, sbinfo.max_use_hash_bucket_ratio_, true, id());
                if (TFS_SUCCESS != ret)
                  ret = index_handle_->update_block_info(oper_type, 0, finfo.size_, true);
                else
                  ret = index_handle_->flush();
              }
            }
            if (TFS_SUCCESS == ret)
            {
              size = finfo.size_ - sizeof(FileInfoInDiskExt);
            }
          }
        }
      }
      return ret;
    }

    int LogicBlock::scan(std::vector<FileInfo>& finfos, const bool sort, const uint64_t logic_block_id) const
    {
      int32_t ret = (INVALID_BLOCK_ID != logic_block_id) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        ret = index_handle_->traverse(finfos, sort, logic_block_id);
      }
      return ret;
    }

    int LogicBlock::scan(std::vector<FileInfoV2>& finfos, const bool sort, const uint64_t logic_block_id) const
    {
      int32_t ret = (INVALID_BLOCK_ID != logic_block_id) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        ret = index_handle_->traverse(finfos, sort, logic_block_id);
      }
      return ret;
    }

    int LogicBlock::get_block_info(BlockInfoV2& info, const uint64_t logic_block_id) const
    {
      int32_t ret = (INVALID_BLOCK_ID != logic_block_id) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        ret = index_handle_->get_block_info(info, logic_block_id);
      }
      return ret;
    }

    int LogicBlock::get_family_id(int64_t& family_id, const uint64_t logic_block_id) const
    {
      family_id = INVALID_FAMILY_ID;
      int32_t ret = (INVALID_BLOCK_ID != logic_block_id) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        ret = index_handle_->get_family_id(family_id, logic_block_id);
      }
      return ret;
    }

    int LogicBlock::set_family_id(const int64_t family_id, const uint64_t logic_block_id)
    {
      int32_t ret = (INVALID_BLOCK_ID != logic_block_id) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        ret = index_handle_->set_family_id(family_id, logic_block_id);
      }
      return ret;
    }

    int LogicBlock::get_used_size(int32_t& size, const uint64_t logic_block_id) const
    {
      int32_t ret = (INVALID_BLOCK_ID != logic_block_id) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        ret = index_handle_->get_used_offset(size, logic_block_id);
      }
      return ret;
    }

    int LogicBlock::get_avail_size(int32_t& size, const uint64_t logic_block_id) const
    {
      int32_t ret = (INVALID_BLOCK_ID != logic_block_id) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        ret = index_handle_->get_avail_offset(size, logic_block_id);
      }
      return ret;
    }

    int LogicBlock::inc_write_visit_count(const int32_t step, const int32_t nbytes, const uint64_t logic_block_id)
    {
      return index_handle_->inc_write_visit_count(step, nbytes, logic_block_id);
    }

    int LogicBlock::inc_read_visit_count(const int32_t step,  const int32_t nbytes, const uint64_t logic_block_id)
    {
      return index_handle_->inc_read_visit_count(step, nbytes, logic_block_id);
    }

    int LogicBlock::inc_update_visit_count(const int32_t step,const int32_t nbytes, const uint64_t logic_block_id)
    {
      return index_handle_->inc_update_visit_count(step, nbytes, logic_block_id);
    }

    int LogicBlock::inc_unlink_visit_count(const int32_t step,const int32_t nbytes, const uint64_t logic_block_id)
    {
      return index_handle_->inc_unlink_visit_count(step, nbytes, logic_block_id);
    }

    int LogicBlock::statistic_visit(common::ThroughputV2& throughput, const bool reset,
            const uint64_t logic_block_id)
    {
      return index_handle_->statistic_visit(throughput, reset, logic_block_id);
    }

    int LogicBlock::extend_block_(const int32_t size, const int32_t offset)
    {
      int32_t ret = (size > 0 && offset >= 0) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        std::string path;
        int32_t avail_offset = 0;
        BlockManager block_manager(path);
        SuperBlockManager& supber_block_manager = block_manager.get_super_block_manager();
        PhysicalBlockManager&  physical_block_manager = block_manager.get_physical_block_manager();
        ret = index_handle_->get_avail_offset(avail_offset, id());
        if (TFS_SUCCESS == ret)
        {
          int32_t retry_times = 30;
          while (TFS_SUCCESS == ret && (offset + size) > avail_offset && retry_times-- > 0)
          {
            BlockIndex index, ext_index;
            ret = (!physical_block_list_.empty()) ? TFS_SUCCESS : EXIT_PHYSICALBLOCK_NUM_ERROR;
            if (TFS_SUCCESS == ret)
            {
              PhysicalBlock* last_physical_block = physical_block_list_.back();
              index.physical_block_id_ = last_physical_block->id();
              ret = (INVALID_PHYSICAL_BLOCK_ID != index.physical_block_id_) ? TFS_SUCCESS : EXIT_PHYSICALBLOCK_NUM_ERROR;
            }
            if (TFS_SUCCESS == ret)
            {
              ret = supber_block_manager.get_block_index(index, index.physical_block_id_);
            }

            if (TFS_SUCCESS == ret)
            {
              ret = physical_block_manager.alloc_ext_block(index, ext_index);
            }
            if (TFS_SUCCESS == ret)
            {
              BasePhysicalBlock* new_physical_block = physical_block_manager.get(ext_index.physical_block_id_);
              ret = (NULL == new_physical_block) ? EXIT_PHYSICAL_BLOCK_NOT_FOUND : TFS_SUCCESS;
              if (TFS_SUCCESS == ret)
              {
                PhysicalBlock* physical_block = dynamic_cast<PhysicalBlock*>(new_physical_block);
                physical_block_list_.push_back(physical_block);
                ret = index_handle_->update_avail_offset(physical_block->length(), id());
                if (TFS_SUCCESS == ret)
                {
                  avail_offset += physical_block->length();
                }
              }
            }
          }
        }
      }
      return ret;
    }

    LogicBlockIterator::LogicBlockIterator(LogicBlock& logic_block):
      logic_block_(logic_block),
      read_disk_offset_(0),
      iter_(logic_block.index_handle_->begin())
    {

    }

    LogicBlockIterator::~LogicBlockIterator()
    {

    }

    bool LogicBlockIterator::empty() const
    {
      int32_t offset = 0;
      int32_t ret = logic_block_.index_handle_->get_used_offset(offset);
      return (TFS_SUCCESS == ret) ? read_disk_offset_ >= offset : true;
    }

    int LogicBlockIterator::transfer_offet_disk_to_mem_(const int32_t disk_offset) const
    {
      int32_t offset = -1, result = -1;
      int32_t ret = logic_block_.index_handle_->get_used_offset(offset);
      if (TFS_SUCCESS == ret)
      {
        if (disk_offset >= read_disk_offset_
            && disk_offset >= 0
            && disk_offset <= offset
            && read_disk_offset_ <= offset)
        {
          result = disk_offset - read_disk_offset_;
        }
      }
      return result;
    }

    int LogicBlockIterator::next(int32_t& mem_offset, FileInfoV2*& info)
    {
      info = NULL;
      mem_offset = 0;
      int32_t offset = 0;
      int32_t ret = !empty() && (iter_ != logic_block_.index_handle_->end()) ? TFS_SUCCESS : EXIT_BLOCK_NO_DATA;
      if (TFS_SUCCESS == ret)
      {
        ret = logic_block_.index_handle_->get_used_offset(offset);
      }
      if (TFS_SUCCESS == ret)
      {
        ret = EXIT_BLOCK_NO_DATA;
        for (; TFS_SUCCESS != ret && !empty() && iter_ != logic_block_.index_handle_->end();iter_++)
        {
          info = iter_;
          ret = info->id_ != INVALID_FILE_ID && info->offset_ > 0 && info->offset_ < offset ? TFS_SUCCESS : EXIT_FILE_EMPTY;
        }
        //double check
        if (TFS_SUCCESS == ret)
        {
          ret = info->id_ != INVALID_FILE_ID && info->offset_ > 0 && info->offset_ < offset ? TFS_SUCCESS : EXIT_FILE_EMPTY;
        }
        if (TFS_SUCCESS == ret)
        {
          mem_offset = transfer_offet_disk_to_mem_(info->offset_);
          ret = mem_offset >= 0 ? TFS_SUCCESS :EXIT_BLOCK_NO_DATA;
        }
        if (TFS_SUCCESS == ret)
        {
          const int32_t disk_data_length = offset - read_disk_offset_;
          const int32_t mem_data_length  = MAX_DATA_SIZE - mem_offset;
          if (mem_data_length < info->size_ && disk_data_length > 0)
          {
            memmove(static_cast<void*>(data_), (data_ + mem_offset), mem_data_length);
            read_disk_offset_ -= mem_data_length;
            int32_t MAX_READ_SIZE = std::min((MAX_DATA_SIZE - mem_data_length), disk_data_length);
            ret = logic_block_.pread((data_ + mem_offset), MAX_READ_SIZE, (read_disk_offset_ + mem_data_length));
            if (TFS_SUCCESS == ret)
            {
              ret = MAX_READ_SIZE >= info->size_ - mem_data_length ? TFS_SUCCESS : EXIT_BLOCK_NO_DATA;
            }
          }
        }
      }
      return ret;
    }

    const common::FileInfoV2& LogicBlockIterator::get_file_info() const
    {
      return (*iter_);
    }

    const char* LogicBlockIterator::get_data(const int32_t mem_offset) const
    {
      return (mem_offset > 0 && mem_offset < MAX_DATA_SIZE) ? (data_ + mem_offset) : NULL;
    }
  }/** end namespace dataserver **/
}/** end namespace tfs **/