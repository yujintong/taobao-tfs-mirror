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

#include <write_file_message_v2.h>

using namespace tfs::common;

namespace tfs
{
  namespace message
  {
    WriteFileMessageV2::WriteFileMessageV2():
      lease_id_(INVALID_LEASE_ID), flag_(INVALID_FLAG), data_(NULL)
    {
      _packetHeader._pcode = WRITE_FILE_MESSAGE_V2;
    }

    WriteFileMessageV2::~WriteFileMessageV2()
    {
    }

    int WriteFileMessageV2::serialize(Stream& output) const
    {
      int64_t pos = 0;
      int ret = file_seg_.serialize(output.get_free(), output.get_free_length(), pos);
      if (TFS_SUCCESS == ret)
      {
        ret = output.pour(file_seg_.length());
      }

      if (TFS_SUCCESS == ret)
      {
        ret = output.set_int64(lease_id_);
      }

      if (TFS_SUCCESS == ret)
      {
        ret = output.set_int32(flag_);
      }

      if (TFS_SUCCESS == ret)
      {
        ret = output.set_vint64(ds_);
      }

      if (TFS_SUCCESS == ret && file_seg_.length_ > 0)
      {
        ret = output.set_bytes(data_, file_seg_.length_);
      }

      return ret;
    }

    int WriteFileMessageV2::deserialize(Stream& input)
    {
      int64_t pos = 0;
      int ret = file_seg_.deserialize(input.get_data(), input.get_data_length(), pos);
      if (TFS_SUCCESS == ret)
      {
        ret = input.drain(file_seg_.length());
      }

      if (TFS_SUCCESS == ret)
      {
        ret = input.get_int64(reinterpret_cast<int64_t *>(&lease_id_));
      }

      if (TFS_SUCCESS == ret)
      {
        input.get_int32(&flag_);
      }

      if (TFS_SUCCESS == ret)
      {
        ret = input.get_vint64(ds_);
      }

      if (TFS_SUCCESS == ret && file_seg_.length_ > 0)
      {
        data_ = input.get_data();
        input.drain(file_seg_.length_);
      }

      return ret;
    }

    int64_t WriteFileMessageV2::length() const
    {
      int64_t len = file_seg_.length() +
                    Serialization::get_vint64_length(ds_) +
                    INT64_SIZE + INT_SIZE;
      if (file_seg_.length_ > 0)
      {
        len += (INT_SIZE + file_seg_.length_);
      }

      return len;
    }

    WriteFileRespMessageV2::WriteFileRespMessageV2():
      file_id_(0), lease_id_(INVALID_LEASE_ID)
    {
      _packetHeader._pcode = WRITE_FILE_RESP_MESSAGE_V2;
    }

    WriteFileRespMessageV2::~WriteFileRespMessageV2()
    {
    }

    int WriteFileRespMessageV2::serialize(Stream& output) const
    {
      int ret = output.set_int64(file_id_);
      if (TFS_SUCCESS == ret)
      {
        ret = output.set_int64(lease_id_);
      }
      return ret;
    }

    int WriteFileRespMessageV2::deserialize(Stream& input)
    {
      int ret = input.get_int64(reinterpret_cast<int64_t *>(&file_id_));
      if (TFS_SUCCESS == ret)
      {
        ret = input.get_int64(reinterpret_cast<int64_t *>(&lease_id_));
      }
      return ret;
    }

    int64_t WriteFileRespMessageV2::length() const
    {
      return 2 * INT64_SIZE;
    }

    SlaveDsRespMessage::SlaveDsRespMessage():
      server_id_(INVALID_SERVER_ID)
    {
      _packetHeader._pcode = SLAVE_DS_RESP_MESSAGE;
    }

    SlaveDsRespMessage::~SlaveDsRespMessage()
    {
    }

    int SlaveDsRespMessage::serialize(Stream& output) const
    {
      int ret = output.set_int64(server_id_);
      if (TFS_SUCCESS == ret)
      {
        int64_t pos = 0;
        ret = block_info_.serialize(output.get_free(), output.get_free_length(), pos);
        if (TFS_SUCCESS == ret)
        {
          output.pour(block_info_.length());
        }
      }

      if (TFS_SUCCESS == ret)
      {
        ret = output.set_int32(status_);
      }
      return ret;
    }

    int SlaveDsRespMessage::deserialize(Stream& input)
    {
      int ret = input.get_int64(reinterpret_cast<int64_t *>(server_id_));
      if (TFS_SUCCESS == ret)
      {
        int64_t pos = 0;
        ret = block_info_.deserialize(input.get_data(), input.get_data_length(), pos);
        if (TFS_SUCCESS == ret)
        {
          input.drain(block_info_.length());
        }
      }

      if (TFS_SUCCESS == ret)
      {
        ret = input.get_int32(&status_);
      }

      return ret;
    }

    int64_t SlaveDsRespMessage::length() const
    {
      return (INT64_SIZE + INT_SIZE + block_info_.length());
    }

  }
}
