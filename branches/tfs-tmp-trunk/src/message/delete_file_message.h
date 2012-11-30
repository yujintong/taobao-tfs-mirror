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

#ifndef TFS_MESSAGE_DELETEFILEMESSAGE_H_
#define TFS_MESSAGE_DELETEFILEMESSAGE_H_

#include <common/base_packet.h>
#include <common/status_message.h>

namespace tfs
{
  namespace message
  {
    class DeleteFileMessage: public common::BasePacket
    {
      public:
        DeleteFileMessage();
        virtual ~DeleteFileMessage();
        virtual int serialize(common::Stream& output) const ;
        virtual int deserialize(common::Stream& input);
        virtual int64_t length() const;

        void set_block_id(const uint64_t block_id)
        {
          block_id_ = block_id;
        }

        uint64_t get_block_id() const
        {
          return block_id_;
        }

        void set_file_id(const uint64_t file_id)
        {
          file_id_ = file_id;
        }

        uint64_t get_file_id() const
        {
          return file_id_;
        }

        void set_ds(const common::VUINT64& ds)
        {
          ds_ = ds;
        }

        const common::VUINT64& get_ds() const
        {
          return ds_;
        }

        void set_action(const int32_t action)
        {
          action_ = action;
        }

        int32_t get_action() const
        {
          return action_;
        }

        void set_flag(const int32_t flag)
        {
          flag_ = flag;
        }

        int32_t get_flag() const
        {
          return flag_;
        }

        void set_family_info(const common::FamilyInfoExt& family_info)
        {
          family_info_ = family_info;
        }

        const common::FamilyInfoExt& get_family_info() const
        {
          return family_info_;
        }

      private:
        uint64_t block_id_;
        uint64_t file_id_;
        common::VUINT64 ds_;
        int32_t action_;
        int32_t flag_;
        common::FamilyInfoExt family_info_;
    };

  }
}
#endif
