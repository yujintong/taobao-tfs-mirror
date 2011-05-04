/*
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
#ifndef TFS_RCSERVER_MYSQL_DATABASE_HELPER_H_
#define TFS_RCSERVER_MYSQL_DATABASE_HELPER_H_
#include <tbsys.h>
#include <Mutex.h>
#include "database_helper.h"
namespace tfs
{
  namespace rcserver
  {
    class MysqlDatabaseHelper :public DatabaseHelper
    {
      public:
        virtual ~MysqlDatabaseHelper();
        virtual int connect();
        virtual int close();

        //ResourceServerInfo 
        virtual int select(const ResourceServerInfo& inparam, ResourceServerInfo& outparam);
        virtual int update(const ResourceServerInfo& inparam);
        virtual int remove(const ResourceServerInfo& inparam);
        virtual int scan(VResourceServerInfo& outparam);

        //AppInfo
        virtual int select(const AppInfo& inparam, AppInfo& outparam);
        virtual int update(const AppInfo& inparam);
        virtual int remove(const AppInfo& inparam);
        virtual int scan(MIdAppInfo& outparam);
      private:
        tbutil::Mutex mutex_;

    };

  }
}
#endif
