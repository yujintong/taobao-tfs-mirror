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
 *   chuyu <chuyu@taobao.com>
 *      - modify 2010-03-20
 *
 */
#include <stdio.h>
#include "common/parameter.h"
#include "dataserver/version.h"
#include "dataserver/superblock.h"
#include "dataserver/blockfile_manager.h"

using namespace tfs::dataserver;
using namespace tfs::common;
using namespace std;

int main(int argc,char* argv[])
{
  char* conf_file = NULL;
  int32_t help_info = 0;
  int32_t i;
  string server_index;

  while ((i = getopt(argc, argv, "f:i:vh")) != EOF) 
  {
    switch (i) 
    {
      case 'f':
        conf_file = optarg; 
        break;
      case 'i':
        server_index = optarg;
        break;
      case 'v':
        fprintf(stderr, "read tfs file system super block tool, version: %s\n", Version::get_build_description());
        return 0;
      case 'h':
      default:
        help_info = 1;
        break;
    }
  }

  if (NULL == conf_file || 0 == server_index.size() || help_info)
  {
    fprintf(stderr, "\nUsage: %s -f conf_file -i index -h -v\n", argv[0]);
    fprintf(stderr, "  -f configure file\n");
    fprintf(stderr, "  -i server_index  dataserver index number\n");
    fprintf(stderr, "  -v show version info\n");
    fprintf(stderr, "  -h help info\n");
    fprintf(stderr, "\n");
    return -1;
  }

  int32_t ret = 0;
  if ((ret = SysParam::instance().load_data_server(conf_file, server_index)) != TFS_SUCCESS) 
  {
    cerr << "SysParam::loadFileSystemParam failed:" << conf_file << endl;
    return ret;
  }

  cout << "mount name: " << SysParam::instance().filesystem_param().mount_name_
    << " max mount size: " << SysParam::instance().filesystem_param().max_mount_size_
    << " base fs type: " << SysParam::instance().filesystem_param().base_fs_type_
    << " superblock reserve offset: " << SysParam::instance().filesystem_param().super_block_reserve_offset_
    << " main block size: " << SysParam::instance().filesystem_param().main_block_size_
    << " extend block size: " << SysParam::instance().filesystem_param().extend_block_size_
    << " block ratio: " << SysParam::instance().filesystem_param().block_type_ratio_
    << " file system version: " << SysParam::instance().filesystem_param().file_system_version_
    << " hash slot ratio: " << SysParam::instance().filesystem_param().hash_slot_ratio_
    << endl;

  ret = BlockFileManager::get_instance()->load_super_blk(SYSPARAM_FILESYSPARAM);
  if (ret)
  {	
    fprintf(stderr, "load tfs file system superblock fail. ret: %d\n",ret);
    return ret;
  }

  SuperBlock super_block;
  ret = BlockFileManager::get_instance()->query_super_block(super_block);
  if (ret)
  {	
    fprintf(stderr, "query superblock fail. ret: %d\n",ret);
    return ret;
  }

  super_block.display();
  return 0;
}
