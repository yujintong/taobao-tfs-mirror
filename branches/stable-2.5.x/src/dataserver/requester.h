/*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: requester.h 356 2011-05-26 07:51:09Z daoan@taobao.com $
 *
 * Authors:
 *   duolong <duolong@taobao.com>
 *      - initial release
 *
 */
#ifndef TFS_DATASERVER_REQUESTER_H_
#define TFS_DATASERVER_REQUESTER_H_

#include "data_management.h"

namespace tfs
{
  namespace dataserver
  {
    class Requester
    {
      public:
        Requester() :
          data_management_(NULL)
        {
        }

        ~Requester()
        {
        }

        int init(DataManagement* data_management);
        int req_update_block_info(const uint32_t block_id, const common::UpdateBlockType repair = common::UPDATE_BLOCK_NORMAL);
        int req_block_write_complete(const uint32_t block_id,
            const int32_t lease_id, const int32_t success, const common::UnlinkFlag unlink_flag = common::UNLINK_FLAG_NO);

      private:
        DISALLOW_COPY_AND_ASSIGN(Requester);

        DataManagement* data_management_;
    };
  }
}

#endif //TFS_DATASERVER_REQUESTER_H_
