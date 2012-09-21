/*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: service.cpp 344 2011-05-26 01:17:38Z duanfei@taobao.com $
 *
 * Authors:
 *   duanfei <duanfei@taobao.com> 
 *      - initial release
 *
 */
#include <exception>
#include <tbsys.h>
#include <Memory.hpp>
#include "rootserver.h"

int main(int argc, char* argv[])
{
  tfs::rootserver::RootServer service;
  return service.main(argc, argv);
}

