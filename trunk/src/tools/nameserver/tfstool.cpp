/*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: tfstool.cpp 1000 2011-11-03 02:40:09Z mingyan.zc@taobao.com $
 *
 * Authors:
 *   duolong <duolong@taobao.com>
 *      - initial release
 *   chuyu <chuyu@taobao.com>
 *      - modify 2010-03-20
 *
 */
#include <stdio.h>
#include <pthread.h>
#include <signal.h>

#include <vector>
#include <string>
#include <map>

#include "tbsys.h"

#include "common/internal.h"
#include "common/config_item.h"
#include "common/client_manager.h"
#include "common/status_message.h"
#include "common/meta_server_define.h"
#include "common/kv_meta_define.h"
#include "common/kv_rts_define.h"
#include "common/expire_define.h"
#include "message/server_status_message.h"
#include "message/client_cmd_message.h"
#include "message/message_factory.h"
#include "message/kv_rts_message.h"
#include "common/base_packet_streamer.h"
#include "tools/util/tool_util.h"
#include "tools/util/ds_lib.h"
#include "clientv2/tfs_client_impl_v2.h"
#include "clientv2/tfs_rc_client_api_impl.h"
#include "clientv2/fsname.h"
#include "common/version.h"


using namespace std;
using namespace tfs::clientv2;
using namespace tfs::common;
using namespace tfs::message;
using namespace tfs::tools;

static TfsClientImplV2* g_tfs_client = NULL;
static STR_FUNC_MAP g_cmd_map;
static const char* dev_name = "bond0";
static const char* app_ip = tbsys::CNetUtil::addrToString(static_cast<uint64_t>(tbsys::CNetUtil::getLocalAddr(dev_name))).c_str();
static const char* default_app_key = "tfscom";
char app_key[256];

typedef enum {
  META_RAW = 0,
  META_NAME,
  META_KV,
  META_LIFECYCLE
} MetaType;

#ifdef _WITH_READ_LINE
#include "readline/readline.h"
#include "readline/history.h"

char* match_cmd(const char* text, int state)
{
  static STR_FUNC_MAP_ITER it;
  static int len = 0;
  const char* cmd = NULL;

  if (!state)
  {
    it = g_cmd_map.begin();
    len = strlen(text);
  }

  while(it != g_cmd_map.end())
  {
    cmd = it->first.c_str();
    it++;
    if (strncmp(cmd, text, len) == 0)
    {
      int32_t cmd_len = strlen(cmd) + 1;
      // memory will be freed by readline
      return strncpy(new char[cmd_len], cmd, cmd_len);
    }
  }
  return NULL;
}

char** tfscmd_completion (const char* text, int start, int)
{
  rl_attempted_completion_over = 1;
  // at the start of line, then it's a cmd completion
  return (0 == start) ? rl_completion_matches(text, match_cmd) : (char**)NULL;
}
#endif

static void sign_handler(const int32_t sig);
static void usage(const char* name);
void init();
int main_loop();
int do_cmd(char* buffer);


int cmd_cd(const VSTRING& param);
int cmd_ls(const VSTRING& param);
int cmd_pwd(const VSTRING& param);
int cmd_show_help(const VSTRING& param);
int cmd_quit(const VSTRING& param);
int cmd_put_file(const VSTRING& param);
int cmd_get_file(const VSTRING& param);
int cmd_remove_file(const VSTRING& param);
int cmd_uremove_file(const VSTRING& param);
int cmd_undel_file(const VSTRING& param);
int cmd_hide_file(const VSTRING& param);
int cmd_stat_file(const VSTRING& param);
int cmd_stat_blk(const VSTRING& param);
int cmd_list_file_info(const VSTRING& param);
int cmd_batch_file(const VSTRING& param);
int cmd_check_file_info(const VSTRING& param);
int cmd_list_block(const VSTRING& param);


//the function of raw tfs
int cmd_put_file_raw(const VSTRING& param);
int cmd_get_file_raw(const VSTRING& param);
int cmd_stat_file_raw(const VSTRING& param);
int cmd_remove_file_raw(const VSTRING& param);
int cmd_unremove_file_raw(const VSTRING& param);
int cmd_hide_file_raw(const VSTRING& param);

int remove_file_raw_ex(const VSTRING& param, TfsUnlinkType type);


// for kv meta server
int cmd_put_bucket(const VSTRING& param);
int cmd_get_bucket(const VSTRING& param);
int cmd_del_bucket(const VSTRING& param);
int cmd_head_bucket(const VSTRING& param);

int cmd_put_object(const VSTRING& param);
int cmd_get_object(const VSTRING& param);
int cmd_del_object(const VSTRING& param);
int cmd_head_object(const VSTRING& param);

/* for lifecycle */
int cmd_set_life_cycle(const VSTRING& param);
int cmd_get_life_cycle(const VSTRING& param);
int cmd_del_life_cycle(const VSTRING& param);

/* for lifecycle root */
int cmd_query_task(const VSTRING& param);

const char* rc_addr = NULL;
const char* nsip = NULL;
const char* krs_addr = NULL;
const char* lrs_addr = NULL;
MetaType g_meta_type = META_RAW;

static tfs::common::BasePacketStreamer gstreamer;
static tfs::message::MessageFactory gfactory;

int main(int argc, char* argv[])
{
  int32_t i;
  int ret = TFS_SUCCESS;
  bool directly = false;
  bool set_log_level = false;

  // analyze arguments
  while ((i = getopt(argc, argv, "s:r:k:l:nihv")) != EOF)
  {
    switch (i)
    {
      case 'n':
        set_log_level = true;
        break;
      case 's':
        nsip = optarg;
        break;
      case 'r':
        rc_addr = optarg;
        break;
      case 'k':
        krs_addr = optarg;
        break;
      case 'l':
        lrs_addr = optarg;
      case 'i':
        directly = true;
        break;
      case 'v':
      case 'h':
      default:
        usage(argv[0]);
        return TFS_ERROR;
    }
  }

  if (set_log_level)
  {
    TBSYS_LOGGER.setLogLevel("ERROR");
  }

  if (NULL == nsip && NULL == rc_addr & NULL == krs_addr && NULL == lrs_addr)
  {
    usage(argv[0]);
    return TFS_ERROR;
  }

  gstreamer.set_packet_factory(&gfactory);
  NewClientManager::get_instance().initialize(&gfactory, &gstreamer);

  if (lrs_addr != NULL)
  {
    if (rc_addr == NULL)
    {
      usage(argv[0]);
      return TFS_ERROR;
    }
    // kv meta use RcClientImpl client
    strcpy(app_key, default_app_key);
    g_meta_type = META_LIFECYCLE;
  }
  else if (krs_addr != NULL)
  {
    if (rc_addr == NULL)
    {
      usage(argv[0]);
      return TFS_ERROR;
    }
    strcpy(app_key, default_app_key);
    g_meta_type = META_KV;
  }
  else if (rc_addr != NULL)
  {
    strcpy(app_key, default_app_key);
    g_meta_type = META_NAME;
  }
  else if (nsip != NULL)
  {
    g_tfs_client = TfsClientImplV2::Instance();
    // ret = g_tfs_client->initialize(nsip, DEFAULT_BLOCK_CACHE_TIME, 1000, false);
    ret = g_tfs_client->initialize(nsip);
    if (ret != TFS_SUCCESS)
    {
      fprintf(stderr, "init tfs client fail, ret: %d\n", ret);
      return ret;
    }
    g_meta_type = META_RAW;
  }

  init();

  if (optind >= argc)
  {
    signal(SIGINT, sign_handler);
    signal(SIGTERM, sign_handler);
    main_loop();
  }
  else // has other params
  {
    int32_t i = 0;
    if (directly)
    {
      for (i = optind; i < argc; i++)
      {
        do_cmd(argv[i]);
      }
    }
    else
    {
      VSTRING param;
      for (i = optind; i < argc; i++)
      {
        param.clear();
        param.push_back(argv[i]);
        cmd_batch_file(param);
      }
    }
  }
  if (g_tfs_client != NULL)
  {
    g_tfs_client->destroy();
  }
  return TFS_SUCCESS;
}

static void usage(const char* name)
{
  fprintf(stderr, "%s\n", Version::get_build_description());
  fprintf(stderr,
          "Usage: a) %s -s nsip [-n] [-i] [-h] raw tfs client interface(without rc). \n"
          "       b) %s -r rcip [-n] [-i] [-h] raw tfs client interface(with rc). \n"
          "       c) %s -k krsip -r rcip [-n] [-i] [-h] kv meta client interface. \n"
          "       D) %s -l lrsip -r rcip [-n] [-i] [-h] lifecycle root client interface. \n"
          "       -s nameserver ip port\n"
          "       -r rcserver ip port\n"
          "       -k kvrootserver ip port\n"
          "       -n set log level\n"
          "       -i directly execute the command\n"
          "       -h help\n",
          name, name, name, name);
}

static void sign_handler(const int32_t sig)
{
  switch (sig)
  {
  case SIGINT:
  case SIGTERM:
    fprintf(stderr, "\nTFS> ");
      break;
  }
}

void init()
{
  g_cmd_map["help"] = CmdNode("help", "show help info", 0, 0, cmd_show_help);
  g_cmd_map["quit"] = CmdNode("quit", "quit", 0, 0, cmd_quit);
  g_cmd_map["exit"] = CmdNode("exit", "exit", 0, 0, cmd_quit);
  g_cmd_map["@"] = CmdNode("@ file", "batch run command in file", 1, 1, cmd_batch_file);
  g_cmd_map["batch"] = CmdNode("batch file", "batch run command in file", 1, 1, cmd_batch_file);
  switch (g_meta_type)
  {
  case META_RAW:
    g_cmd_map["cd"] = CmdNode("cd [directory]", "change work directory", 0, 1, cmd_cd);
    g_cmd_map["ls"] = CmdNode("ls [directory]", "list directory content", 0, 1, cmd_ls);
    g_cmd_map["pwd"] = CmdNode("pwd", "print current directory", 0, 0, cmd_pwd);
    g_cmd_map["put"] = CmdNode("put localfile [tfsname [suffix] [force]]", "put file to tfs", 1, 4, cmd_put_file);
    g_cmd_map["get"] = CmdNode("get tfsname localfile", "get file from tfs", 2, 2, cmd_get_file);
    g_cmd_map["rm"] = CmdNode("rm tfsname", "remove tfs file", 1, 1, cmd_remove_file);
    g_cmd_map["undel"] = CmdNode("undel tfsname", "undelete tfs file", 1, 1, cmd_undel_file);
    g_cmd_map["hide"] = CmdNode("hide tfsname [action]", "hide tfs file", 1, 2, cmd_hide_file);
    g_cmd_map["stat"] = CmdNode("stat tfsname", "stat tfs file", 1, 1, cmd_stat_file);
    g_cmd_map["statblk"] = CmdNode("statblk blockid [serverip:port]", "stat a block", 1, 2, cmd_stat_blk);
    g_cmd_map["lsf"] = CmdNode("lsf blockid [attach_block_id] [detail] [serverip:port]" , "list file list in block", 1, 4, cmd_list_file_info);
    g_cmd_map["listblock"] = CmdNode("listblock blockid", "list block server list", 1, 1, cmd_list_block);
    g_cmd_map["cfi"] = CmdNode("cfi tfsname", "check file info", 1, 1, cmd_check_file_info);
    break;
  case META_NAME:
    g_cmd_map["put"] = CmdNode("put localfile [suffix [app_key]]", "put raw file to tfs", 1, 3, cmd_put_file_raw);
    g_cmd_map["get"] = CmdNode("get tfsname localfile [app_key]", "get raw file from tfs", 2, 3, cmd_get_file_raw);
    g_cmd_map["stat"] = CmdNode("stat tfsname [app_key]", "stat raw tfs file", 1, 2, cmd_stat_file_raw);
    g_cmd_map["rm"] = CmdNode("rm tfsname [app_key]", "remove raw tfs file", 1, 2, cmd_remove_file_raw);
    g_cmd_map["undel"] = CmdNode("undel tfsname [app_key]", "undelete raw tfs file", 1, 2, cmd_unremove_file_raw);
    g_cmd_map["hide"] = CmdNode("hide tfsname [action [app_key]]", "hide raw tfs file, param 4 for hide and 6 for unhide", 1, 3, cmd_hide_file_raw);
    break;
  case META_KV:
    g_cmd_map["put_bucket"] = CmdNode("put_bucket bucket_name owner_id [app_key]", "create a bucket", 2, 3, cmd_put_bucket);
    g_cmd_map["get_bucket"] = CmdNode("get_bucket bucket_name [ prefix start_key delimiter limit app_key]", "get a bucket(list object)", 1, 6, cmd_get_bucket);
    g_cmd_map["del_bucket"] = CmdNode("del_bucket bucket_name [app_key]", "delete a bucket", 1, 2, cmd_del_bucket);
    g_cmd_map["head_bucket"] = CmdNode("head_bucket bucket_name [app_key]", "stat a bucket", 1, 2, cmd_head_bucket);

    g_cmd_map["put_object"] = CmdNode("put_object bucket_name object_name local_file owner_id [app_key]", "put a object", 4, 5, cmd_put_object);
    g_cmd_map["get_object"] = CmdNode("get_object bucket_name object_name local_file [app_key]", "get a object", 3, 4, cmd_get_object);
    g_cmd_map["del_object"] = CmdNode("del_object bucket_name object_name [app_key]", "delete a object", 2, 3, cmd_del_object);
    g_cmd_map["head_object"] = CmdNode("head_object bucket_name object_name [app_key]", "stat a object", 2, 3, cmd_head_object);
    g_cmd_map["set_life_cycle"] = CmdNode("set_life_cycle file_type file_name invalid_time_s app_key", "set a expire time for file", 4, 4, cmd_set_life_cycle);
    g_cmd_map["get_life_cycle"] = CmdNode("get_life_cycle file_type file_name", "get a expire time for a file", 2, 2, cmd_get_life_cycle);
    g_cmd_map["del_life_cycle"] = CmdNode("del_life_cycle file_type file_name", "delete a expire time for a file", 2, 2, cmd_del_life_cycle);
    break;

  case META_LIFECYCLE:
    g_cmd_map["query_task"] = CmdNode("query_task expire_server", "query lifecycle root task", 1, 1, cmd_query_task);
    break;
  }
}

int main_loop()
{
#ifdef _WITH_READ_LINE
  char* cmd_line = NULL;
  rl_attempted_completion_function = tfscmd_completion;
#else
  char cmd_line[MAX_CMD_SIZE];
#endif

  int ret = TFS_ERROR;
  while (1)
  {
#ifdef _WITH_READ_LINE
    cmd_line = readline("TFS> ");
    if (!cmd_line)
#else
      fprintf(stdout, "TFS> ");
    if (NULL == fgets(cmd_line, MAX_CMD_SIZE, stdin))
#endif
    {
      continue;
    }
    ret = do_cmd(cmd_line);
#ifdef _WITH_READ_LINE
    free(cmd_line);
#endif
    if (TFS_CLIENT_QUIT == ret)
    {
      break;
    }
  }
  return TFS_SUCCESS;
}

int32_t do_cmd(char* key)
{
  char* token;
  while (' ' == *key)
  {
    key++;
  }
  token = key + strlen(key);
  while (' ' == *(token - 1) || '\n' == *(token - 1) || '\r' == *(token - 1))
  {
    token--;
  }
  *token = '\0';
  if ('\0' == key[0])
  {
    return TFS_SUCCESS;
  }

#ifdef _WITH_READ_LINE
  // not blank line, add to history
  add_history(key);
#endif

  token = strchr(key, ' ');
  if (token != NULL)
  {
    *token = '\0';
  }

  string cur_cmd = Func::str_to_lower(key);
  STR_FUNC_MAP_ITER it = g_cmd_map.find(cur_cmd);

  if (it == g_cmd_map.end())
  {
    fprintf(stderr, "unknown command. \n");
    return TFS_ERROR;
  }

  if (token != NULL)
  {
    token++;
    key = token;
  }
  else
  {
    key = NULL;
  }

  VSTRING param;
  param.clear();
  while ((token = strsep(&key, " ")) != NULL)
  {
    if ('\0' == token[0])
    {
      continue;
    }
    param.push_back(token);
  }

  // check param count
  int32_t param_cnt = param.size();
  if (param_cnt < it->second.min_param_cnt_ || param_cnt > it->second.max_param_cnt_)
  {
    fprintf(stderr, "%s\t\t%s\n\n", it->second.syntax_, it->second.info_);
    return TFS_ERROR;
  }

  return it->second.func_(param);
}

const char* canonical_param(const string& param)
{
  const char* ret_param = param.c_str();
  if (NULL != ret_param &&
      (strlen(ret_param) == 0 ||
       strcasecmp(ret_param, "null") == 0))
  {
    ret_param = NULL;
  }
  return ret_param;
}

// expand ~ to HOME. modify argument
const char* expand_path(string& path)
{
  if (path.size() > 0 && '~' == path.at(0) &&
      (1 == path.size() ||                      // just one ~
       (path.size() > 1 && '/' == path.at(1)))) // like ~/xxx
  {
    char* home_path = getenv("HOME");
    if (NULL == home_path)
    {
      fprintf(stderr, "can't get HOME path: %s\n", strerror(errno));
    }
    else
    {
      path.replace(0, 1, home_path);
    }
  }
  return path.c_str();
}




int cmd_ls(const VSTRING& param)
{
  int32_t size = param.size();
  const char* path = (1 == size) ? param.at(0).c_str() : ".";
  char sys_cmd[MAX_CMD_SIZE];
  // just use system tool ls, maybe DIY
  snprintf(sys_cmd, MAX_CMD_SIZE, "ls -FCl %s", path);
  return system(sys_cmd);
}

int cmd_pwd(const VSTRING&)
{
  char dir[MAX_PATH_LENGTH], *path = NULL;

  path = getcwd(dir, MAX_PATH_LENGTH - 1);
  if (NULL == path)
  {
    fprintf (stderr, "can't get current work directory: %s\n", strerror(errno));
    return TFS_ERROR;
  }
  fprintf(stdout, "%s\n", dir);
  return TFS_SUCCESS;
}

int cmd_cd(const VSTRING& param)
{
  int ret = TFS_SUCCESS;
  int32_t size = param.size();
  const char* dest_dir = (1 == size) ? expand_path(const_cast<string&>(param[0])) : getenv("HOME");

  if (NULL == dest_dir)
  {
    fprintf(stderr, "no directory argument and HOME not found\n\n");
    ret = TFS_ERROR;
  }
  else if (chdir(dest_dir) == -1)
  {
    fprintf(stderr, "can't change directory %s: %s\n", dest_dir, strerror(errno));
    ret = TFS_ERROR;
  }
  cmd_pwd(param);
  return ret;
}

int cmd_quit(const VSTRING&)
{
  return TFS_CLIENT_QUIT;
}

int cmd_show_help(const VSTRING&)
{
  return ToolUtil::show_help(g_cmd_map);
}

int cmd_put_file(const VSTRING& param)
{
  int32_t size = param.size();
  const char* local_file = expand_path(const_cast<string&>(param[0]));
  const char* tfs_name = NULL;
  const char* suffix = NULL;
  int32_t flag = T_DEFAULT;
  int ret = TFS_SUCCESS;
  char ret_tfs_name[TFS_FILE_LEN_V2];
  ret_tfs_name[0] = '\0';

  if (size > 1)
  {
    tfs_name = canonical_param(param[1]);
  }

  if (size > 2)
  {
    suffix = canonical_param(param[2]);
  }

  if (size > 3 &&
      param[3] == "force")
  {
    flag |= T_NEWBLK;
  }

  if (NULL != tfs_name)  // update
  {
    ret = g_tfs_client->save_file_update(local_file, flag, tfs_name, suffix);
  }
  else
  {
    ret = g_tfs_client->save_file(ret_tfs_name, TFS_FILE_LEN_V2, local_file, flag, suffix);
  }

  if (ret >= 0)
  {
    TBSYS_LOG(DEBUG, "save %d bytes data to tfs", ret);
  }
  ret = (ret < 0) ? ret : TFS_SUCCESS;

  //printf("tfs_name: %s, ret_tfs_name: %s\n", tfs_name, ret_tfs_name);
  ToolUtil::print_info(ret, "put %s => %s", local_file, tfs_name != NULL ? FSName(tfs_name, suffix).get_name() : ret_tfs_name);
  return ret;
}

int cmd_put_file_raw(const VSTRING& param)
{
  int32_t size = param.size();
  const char* local_file = expand_path(const_cast<string&>(param[0]));
  char* tfs_name = NULL;
  const char* suffix = NULL;
  char appkey[257];
  //int32_t flag = T_DEFAULT;
  int ret = TFS_SUCCESS;
  char ret_tfs_name[TFS_FILE_LEN_V2];
  ret_tfs_name[0] = '\0';

  if (size > 1)
  {
    suffix = canonical_param(param[1]);
  }

  if (size > 2)
  {
    //TBSYS_LOG(DEBUG, "app_key: %s", param[2].c_str());
    strncpy(appkey, canonical_param(param[2]), 256);
    appkey[256] = '\0';
  }
  else
  {
    // default app_key = "tfscom"
    strcpy(appkey, app_key);
  }

  //login with rc and app_key
  RcClientImpl impl;
  ret = impl.initialize(rc_addr, appkey, app_ip);

  if (TFS_SUCCESS != ret)
  {
    TBSYS_LOG(DEBUG, "rc client init failed, ret: %d", ret);
  }
  else
  {
    ret = impl.save_file(local_file, ret_tfs_name, TFS_FILE_LEN_V2, suffix) < 0 ? TFS_ERROR : TFS_SUCCESS;
  }

  //printf("tfs_name: %s, ret_tfs_name: %s\n", tfs_name, ret_tfs_name);

  ToolUtil::print_info(ret, "put %s => %s", local_file, tfs_name != NULL ? FSName(tfs_name, suffix).get_name() : ret_tfs_name);
  return ret;
}

int cmd_get_file(const VSTRING& param)
{
  const char* tfs_name = canonical_param(param[0]);
  const char* local_file = expand_path(const_cast<string&>(param[1]));

  int ret = g_tfs_client->fetch_file(local_file, tfs_name, NULL, READ_DATA_OPTION_FLAG_FORCE);

  ToolUtil::print_info(ret, "fetch %s => %s", tfs_name, local_file);

  return ret;
}

int cmd_get_file_raw(const VSTRING& param)
{
  const char* tfs_name = canonical_param(param[0]);
  const char* local_file = expand_path(const_cast<string&>(param[1]));
  char appkey[257];
  int size = param.size();
  if (size > 2)
  {
    TBSYS_LOG(DEBUG, "appkey: %s", param[2].c_str());
    strncpy(appkey, param[2].c_str(), 256);
    appkey[256] = '\0';
  }
  else
  {
    strcpy(appkey, app_key);
  }
  int ret = TFS_SUCCESS;
  RcClientImpl impl;
  ret = impl.initialize(rc_addr, appkey, app_ip);

  if (TFS_SUCCESS != ret)
  {
    TBSYS_LOG(DEBUG, "rc client init failed, ret: %d", ret);
  }
  else
  {
    ret = impl.fetch_file(local_file, tfs_name, NULL);
  }

  ToolUtil::print_info(ret, "fetch %s => %s", tfs_name, local_file);

  return ret;
}


int cmd_remove_file(const VSTRING& param)
{
  const char* tfs_name = canonical_param(param[0]);
  int ret = TFS_SUCCESS;
  int64_t file_size = 0;
  ret = g_tfs_client->unlink(file_size, tfs_name, NULL);

  ToolUtil::print_info(ret, "unlink %s", tfs_name);
  return ret;
}

int cmd_undel_file(const VSTRING& param)
{
  const char* tfs_name = canonical_param(param[0]);
  int64_t file_size = 0;
  int ret = g_tfs_client->unlink(file_size, tfs_name, NULL, UNDELETE);

  ToolUtil::print_info(ret, "undel %s", tfs_name);

  return TFS_SUCCESS;
}

int cmd_remove_file_raw(const VSTRING& param)
{
  return remove_file_raw_ex(param, static_cast<TfsUnlinkType>(0));
}

int cmd_unremove_file_raw(const VSTRING& param)
{
  return remove_file_raw_ex(param, static_cast<TfsUnlinkType>(2));
}

int remove_file_raw_ex(const VSTRING& param, TfsUnlinkType type)
{
  const char* tfs_name = canonical_param(param[0]);
  char appkey[257];
  int ret = TFS_SUCCESS;


  int size = param.size();
  if (size > 1)
  {
    //TBSYS_LOG(DEBUG, "app_key: %s", param[1].c_str());
    strncpy(appkey, canonical_param(param[1]), 256);
    appkey[256] = '\0';
  }
  else
  {
    //default app_key = "tfscom"
    strcpy(appkey, app_key);
  }

  RcClientImpl impl;
  ret = impl.initialize(rc_addr, appkey, app_ip);
  if (TFS_SUCCESS != ret)
  {
    TBSYS_LOG(DEBUG, "rc client init fail, ret: %d", ret);
  }
  else
  {
    ret = impl.unlink(tfs_name, NULL, type);
  }

  if (type == DELETE)
  {
    ToolUtil::print_info(ret, "del %s", tfs_name);
  }
  else if (type == UNDELETE)
  {
    ToolUtil::print_info(ret, "undel %s", tfs_name);
  }

  return ret;
}

int cmd_hide_file(const VSTRING& param)
{
  const char* tfs_name = canonical_param(param[0]);

  TfsUnlinkType unlink_type = CONCEAL;
  if (param.size() > 1)
  {
    unlink_type = static_cast<TfsUnlinkType>(atoi(param[1].c_str()));
  }

  int64_t file_size = 0;
  int ret = g_tfs_client->unlink(file_size, tfs_name, NULL, unlink_type);

  ToolUtil::print_info(ret, "hide %s %d", tfs_name, unlink_type);

  return ret;
}

int cmd_hide_file_raw(const VSTRING& param)
{
  const char* tfs_name = canonical_param(param[0]);
  char appkey[257];
  int size = param.size();

  TfsUnlinkType unlink_type = CONCEAL;
  if (size > 1)
  {
    unlink_type = static_cast<TfsUnlinkType>(atoi(param[1].c_str()));
  }

  if (size > 2)
  {
    strncpy(appkey, param[2].c_str(), 256);
    appkey[256] = '\0';
  }
  else
  {
    strcpy(appkey, app_key);
  }

  RcClientImpl impl;
  int ret = impl.initialize(rc_addr, appkey, app_ip);
  if (TFS_SUCCESS != ret)
  {
    TBSYS_LOG(DEBUG, "rc client login fail, ret: %d", ret);
  }
  else
  {
    ret = impl.unlink(tfs_name, NULL, unlink_type);
  }

  ToolUtil::print_info(ret, "hide %s %d", tfs_name, unlink_type);

  return ret;
}

int cmd_stat_file(const VSTRING& param)
{
  const char* tfs_name = canonical_param(param[0]);

  TfsFileStat file_stat;
  int ret = g_tfs_client->stat_file(&file_stat, tfs_name, NULL, FORCE_STAT);

  ToolUtil::print_info(ret, "stat %s", tfs_name);

  if (TFS_SUCCESS == ret)
  {
    tfs::clientv2::FSName fsname(tfs_name, NULL);
    fprintf(stdout,
            "  FILE_NAME:     %s\n"
            "  BLOCK_ID:      %"PRI64_PREFIX"u\n"
            "  FILE_ID:       %" PRI64_PREFIX "u\n"
            "  OFFSET:        %d\n"
            "  SIZE:          %"PRI64_PREFIX"d\n"
            "  OCCUPY SIZE:   %"PRI64_PREFIX"d\n"
            "  MODIFIED_TIME: %s\n"
            "  CREATE_TIME:   %s\n"
            "  STATUS:        %d\n"
            "  CRC:           %u\n",
            tfs_name, fsname.get_block_id(), file_stat.file_id_,
            file_stat.offset_, file_stat.size_, file_stat.usize_,
            Func::time_to_str(file_stat.modify_time_).c_str(),
            Func::time_to_str(file_stat.create_time_).c_str(),
            file_stat.flag_, file_stat.crc_);
  }

  return ret;
}

int cmd_stat_file_raw(const VSTRING& param)
{
  const char* tfs_name = canonical_param(param[0]);
  char appkey[257];
  int size = param.size();

  if (size > 1)
  {
    //TBSYS_LOG(DEBUG, "app_key: %s", param[1].c_str());
    strncpy(appkey, param[1].c_str(), 256);
    appkey[256] = '\0';
  }
  else
  {
    strcpy(appkey, app_key);
  }

  TfsFileStat file_stat;

  RcClientImpl impl;
  int ret = impl.initialize(rc_addr, appkey, app_ip);
  if (TFS_SUCCESS != ret)
  {
    TBSYS_LOG(DEBUG, "rc init fail, ret: %d", ret);
  }
  else
  {
    int fd = -1;
    if (tfs_name[0] == 'L')
    {
      TBSYS_LOG(DEBUG, "not support large file");
    }
    else
    {
      fd = impl.open(tfs_name, NULL, RcClient::READ);
    }
    if (fd < 0)
    {
      ret = TFS_ERROR;
      TBSYS_LOG(DEBUG, "%s open fail, return fd: %d", tfs_name, fd);
    }
    else
    {
      if ((ret = impl.fstat(fd, &file_stat, FORCE_STAT)) != TFS_SUCCESS)
      {
        TBSYS_LOG(DEBUG, "stat %s fail, return %d", tfs_name, ret);
      }
      int re = impl.close(fd, const_cast<char*>(tfs_name), TFS_FILE_LEN_V2);
      if (TFS_SUCCESS != re)
      {
        TBSYS_LOG(DEBUG, "close %s fail, return %d", tfs_name, re);
      }
    }
  }

  ToolUtil::print_info(ret, "stat %s", tfs_name);

  if (TFS_SUCCESS == ret)
  {
    FSName fsname(tfs_name, NULL);
    fprintf(stdout,
            "  FILE_NAME:     %s\n"
            "  BLOCK_ID:      %"PRI64_PREFIX"u\n"
            "  FILE_ID:       %" PRI64_PREFIX "u\n"
            "  OFFSET:        %d\n"
            "  SIZE:          %"PRI64_PREFIX"d\n"
            "  OCCUPY SIZE:   %"PRI64_PREFIX"d\n"
            "  MODIFIED_TIME: %s\n"
            "  CREATE_TIME:   %s\n"
            "  STATUS:        %d\n"
            "  CRC:           %u\n",
            tfs_name, fsname.get_block_id(), file_stat.file_id_,
            file_stat.offset_, file_stat.size_, file_stat.usize_,
            Func::time_to_str(file_stat.modify_time_).c_str(),
            Func::time_to_str(file_stat.create_time_).c_str(),
            file_stat.flag_, file_stat.crc_);
  }

  // uint64_t id = file_stat.size_;
  // id <<= 32;
  // id |= file_info.crc_;
  //  uint32_t tindex = static_cast<uint32_t>(id & 0xFFFFFFFF);
  //  printf("  TABLE:         select * from t%d where id='%" PRI64_PREFIX "u'\n", tindex % TABLE_COUNT, id);

  return ret;
}

int cmd_batch_file(const VSTRING& param)
{
  const char* batch_file = expand_path(const_cast<string&>(param[0]));
  FILE* fp = fopen(batch_file, "rb");
  int ret = TFS_SUCCESS;
  if (fp == NULL)
  {
    fprintf(stderr, "open file error: %s\n\n", batch_file);
    ret = TFS_ERROR;
  }
  else
  {
    int32_t error_count = 0;
    int32_t count = 0;
    VSTRING params;
    char buffer[MAX_CMD_SIZE];
    while (fgets(buffer, MAX_CMD_SIZE, fp))
    {
      if ((ret = do_cmd(buffer)) == TFS_ERROR)
      {
        error_count++;
      }
      if (++count % 100 == 0)
      {
        fprintf(stdout, "tatol: %d, %d errors.\r", count, error_count);
        fflush(stdout);
      }
      if (TFS_CLIENT_QUIT == ret)
      {
        break;
      }
    }
    fprintf(stdout, "tatol: %d, %d errors.\n\n", count, error_count);
    fclose(fp);
  }
  return TFS_SUCCESS;
}

int cmd_stat_blk(const VSTRING& param)
{
  int ret = TFS_ERROR;

  uint64_t server_id = 0;
  uint64_t block_id = 0;

  if ((block_id = strtoull(param[0].c_str(), (char**)NULL, 10)) <= 0)
  {
    fprintf(stderr, "invalid blockid: %"PRI64_PREFIX"u\n", block_id);
  }

  if (param.size() >= 2)
  {
    server_id = Func::get_host_ip(param[1].c_str());
  }
  else
  {
    VUINT64 ds_list;
    ret = ToolUtil::get_block_ds_list_v2(g_tfs_client->get_server_id(), block_id, ds_list);
    if (ret != TFS_SUCCESS)
    {
      fprintf(stderr, "get ds list failed. block_id: %"PRI64_PREFIX"u, ret: %d\n", block_id, ret);
      return ret;
    }
    server_id = ds_list[0];
  }

  DsTask ds_task(server_id, g_tfs_client->get_cluster_id());
  ds_task.block_id_ = block_id;
  ret = DsLib::get_block_info(ds_task);

  return ret;
}

int cmd_list_file_info(const VSTRING& param)
{
  uint64_t block_id = 0;
  uint64_t attach_block_id = 0;
  uint64_t server_id = 0;

  int32_t show_detail = 0;

  int ret = TFS_ERROR;
  if ((block_id = strtoull(param[0].c_str(), (char**)NULL, 10)) <= 0)
  {
    fprintf(stderr, "invalid blockid: %"PRI64_PREFIX"u\n", block_id);
    return ret;
  }

  if (param.size() > 3)
  {
    server_id = Func::get_host_ip(param[3].c_str());
  }
  else
  {
    VUINT64 ds_list;
    ret = ToolUtil::get_block_ds_list_v2(g_tfs_client->get_server_id(), block_id, ds_list);
    if (ret != TFS_SUCCESS)
    {
      fprintf(stderr, "get ds list failed. block_id: %"PRI64_PREFIX"u, ret: %d\n", block_id, ret);
      return ret;
    }
    server_id = ds_list[0];
  }

  if (0 != server_id)
  {
    if (param.size() > 2 && 0 == strcmp(param[2].c_str(), "detail"))
    {
      show_detail = 1;
    }

    if (param.size() > 1)
    {
      attach_block_id = strtoull(param[1].c_str(), (char**)NULL, 10);
    }
    else
    {
      attach_block_id = block_id;
    }

    DsTask ds_task(server_id, g_tfs_client->get_cluster_id());
    ds_task.block_id_ = block_id;
    ds_task.attach_block_id_ = attach_block_id;
    ds_task.mode_ = show_detail;
    ret = DsLib::list_file(ds_task);
  }

  return ret;
}

int cmd_list_block(const VSTRING& param)
{
  uint64_t block_id = strtoull(param[0].c_str(), (char**)NULL, 10);
  int ret = TFS_ERROR;

  if (block_id <= 0)
  {
    fprintf(stderr, "invalid block id: %"PRI64_PREFIX"u\n", block_id);
  }
  else
  {
    VUINT64 ds_list;
    ret = ToolUtil::get_block_ds_list_v2(g_tfs_client->get_server_id(), block_id, ds_list);
    ToolUtil::print_info(ret, "list block %u", block_id);

    if (TFS_SUCCESS == ret)
    {
      int32_t ds_size = ds_list.size();
      fprintf(stdout, "------block: %"PRI64_PREFIX"u, has %d replicas------\n", block_id, ds_size);
      for (int32_t i = 0; i < ds_size; ++i)
      {
        fprintf(stdout, "block: %"PRI64_PREFIX"u, (%d)th server: %s \n", block_id, i, tbsys::CNetUtil::addrToString(ds_list[i]).c_str());
      }
    }
  }
  return ret;
}

int cmd_check_file_info(const VSTRING& param)
{
  int ret = TFS_ERROR;
  FSName fsname(canonical_param(param[0]));

  if (!fsname.is_valid())
  {
    fprintf(stderr, "file name is invalid: %s\n", param[0].c_str());
  }
  else
  {
    VUINT64 ds_list;
    ret = ToolUtil::get_block_ds_list_v2(g_tfs_client->get_server_id(), fsname.get_block_id(), ds_list);
    if (ret != TFS_SUCCESS)
    {
      fprintf(stderr, "get block info fail, ret: %d\n", ret);
    }
    else
    {
      DsTask ds_task(0, g_tfs_client->get_cluster_id());
      int32_t ds_size = ds_list.size();
      for (int32_t i = 0; i < ds_size; ++i)
      {
        ds_task.server_id_ = ds_list[i];
        ds_task.block_id_ = fsname.get_block_id();
        ds_task.attach_block_id_ = fsname.get_block_id();
        ds_task.new_file_id_ = fsname.get_file_id();
        ret = DsLib::check_file_info(ds_task);
      }
    }
  }
  return ret;
}




// kv meta

int cmd_put_bucket(const VSTRING& param)
{
  const char* bucket_name = param[0].c_str();
  int64_t owner_id = strtoll(param[1].c_str(), NULL, 10);
  char appkey[257];
  int size = param.size();
  if (size > 2)
  {
    strncpy(appkey, param[2].c_str(), 256);
    appkey[256] = '\0';
  }
  else
  {
    strcpy(appkey, app_key);
  }

  UserInfo user_info;
  user_info.owner_id_ = owner_id;

  RcClientImpl impl;
  impl.set_kv_rs_addr(krs_addr);
  int ret = impl.initialize(rc_addr, appkey, app_ip);
  if (TFS_SUCCESS != ret)
  {
    TBSYS_LOG(DEBUG, "rc client init failed, ret: %d", ret);
  }
  else
  {
    ret = impl.put_bucket(bucket_name, user_info);
  }
  if (TFS_SUCCESS == ret)
  {
    ToolUtil::print_info(ret, "put bucket %s owner_id : %ld", bucket_name, owner_id);
  }
  return ret;
}

int cmd_get_bucket(const VSTRING& param)
{
  int size = param.size();

  int ret = TFS_SUCCESS;
  const char *bucket_name = NULL;
  const char *prefix = NULL;
  const char *start_key = NULL;
  char delimiter = KvDefine::DEFAULT_CHAR;
  int32_t limit = KvDefine::MAX_LIMIT;

  bucket_name = param[0].c_str();

  char appkey[257];

  if (size > 1)
  {
    prefix = canonical_param(param[1]);
  }

  if (size > 2)
  {
    start_key = canonical_param(param[2]);
  }

  if (size > 3)
  {
    delimiter = canonical_param(param[3]) == NULL ? KvDefine::DEFAULT_CHAR : (param[3].size() == 1 ? param[3][0] : KvDefine::DEFAULT_CHAR);
  }

  if (size > 4)
  {
    limit = atoi(param[4].c_str());
  }

  if (size > 5)
  {
    strncpy(appkey, param[5].c_str(), 256);
    appkey[256] = '\0';
  }
  else
  {
    strcpy(appkey, app_key);
  }

  vector<ObjectMetaInfo> v_object_meta_info;
  VSTRING v_object_name;
  set<string> s_common_prefix;
  int8_t is_truncated = 0;
  UserInfo user_info;

  RcClientImpl impl;
  impl.set_kv_rs_addr(krs_addr);
  ret = impl.initialize(rc_addr, appkey, app_ip);
  if (TFS_SUCCESS != ret)
  {
    TBSYS_LOG(DEBUG, "rc client init failed, ret: %d", ret);
  }
  else
  {
    ret = impl.get_bucket(bucket_name, prefix, start_key, delimiter, limit,
        &v_object_meta_info, &v_object_name, &s_common_prefix, &is_truncated, user_info);
  }

  if (TFS_SUCCESS == ret)
  {
    printf("bucket: %s has %d common_prefix\n", bucket_name, static_cast<int>(s_common_prefix.size()));
    set<string>::iterator iter = s_common_prefix.begin();
    for (int i = 0; iter != s_common_prefix.end(); iter++, i++)
    {
      cout << i << ": " << *iter << endl;
    }
  }

  if (TFS_SUCCESS == ret)
  {
    printf("bucket: %s has %d objects\n", bucket_name, static_cast<int>(v_object_name.size()));
    for (int i = 0; i < static_cast<int>(v_object_name.size()); i++)
    {
      cout << i << ": " << v_object_name[i] << endl;
    }
  }

  //todo show info of objects
  ToolUtil::print_info(ret, "get bucket %s", bucket_name);
  return ret;
}

int cmd_del_bucket(const VSTRING& param)
{
  const char* bucket_name = param[0].c_str();
  UserInfo user_info;
  char appkey[257];
  int size = param.size();
  if (size > 1)
  {
    strncpy(appkey, param[1].c_str(), 256);
    appkey[256] = '\0';
  }
  else
  {
    strcpy(appkey, app_key);
  }

  RcClientImpl impl;
  impl.set_kv_rs_addr(krs_addr);
  int ret = impl.initialize(rc_addr, appkey, app_ip);

  if (TFS_SUCCESS != ret)
  {
    TBSYS_LOG(DEBUG, "rc client init failed, ret: %d", ret);
  }
  else
  {
    ret = impl.del_bucket(bucket_name, user_info);
  }

  ToolUtil::print_info(ret, "del bucket %s", bucket_name);

  return ret;
}

int cmd_head_bucket(const VSTRING& param)
{
  const char* bucket_name = param[0].c_str();
  char appkey[257];
  int size = param.size();
  if (size > 1)
  {
    strncpy(appkey, param[1].c_str(), 256);
    appkey[256] = '\0';
  }
  else
  {
    strcpy(appkey, app_key);
  }

  BucketMetaInfo bucket_meta_info;
  UserInfo user_info;

  RcClientImpl impl;
  impl.set_kv_rs_addr(krs_addr);
  int ret = impl.initialize(rc_addr, appkey, app_ip);

  if (TFS_SUCCESS != ret)
  {
    TBSYS_LOG(DEBUG, "rc client init failed, ret: %d", ret);
  }
  else
  {
    ret = impl.head_bucket(bucket_name, &bucket_meta_info, user_info);
  }

  ToolUtil::print_info(ret, "head bucket %s", bucket_name);

  if (TFS_SUCCESS == ret)
  {
    printf("bucket: %s, create_time: %"PRI64_PREFIX"d, owner_id: %"PRI64_PREFIX"d\n",
        bucket_name, bucket_meta_info.create_time_, bucket_meta_info.owner_id_);
  }

  return ret;
}


int cmd_put_object(const VSTRING& param)
{
  const char* bucket_name = param[0].c_str();
  const char* object_name = param[1].c_str();
  const char* local_file = expand_path(const_cast<string&>(param[2]));
  int64_t owner_id = strtoll(param[3].c_str(), NULL, 10);
  char appkey[257];
  int size = param.size();
  if (size > 4)
  {
    strncpy(appkey, param[4].c_str(), 256);
    appkey[256] = '\0';
  }
  else
  {
    strcpy(appkey, app_key);
  }

  UserInfo user_info;
  user_info.owner_id_ = owner_id;

  RcClientImpl impl;
  impl.set_kv_rs_addr(krs_addr);
  int ret = impl.initialize(rc_addr, appkey, app_ip);

  if (TFS_SUCCESS != ret)
  {
    TBSYS_LOG(DEBUG, "rc client init failed, ret: %d", ret);
  }
  else
  {
    ret = impl.put_object(bucket_name, object_name, local_file, user_info);
    ToolUtil::print_info(ret, "put object: %s, object: %s => %s owner_id: %"PRI64_PREFIX"d",
        bucket_name, object_name, local_file, owner_id);
  }
  return ret;
}

int cmd_get_object(const VSTRING& param)
{
  const char* bucket_name = param[0].c_str();
  const char* object_name = param[1].c_str();
  const char* local_file = expand_path(const_cast<string&>(param[2]));
  char appkey[257];
  int size = param.size();
  if (size > 3)
  {
    strncpy(appkey, param[3].c_str(), 256);
    appkey[256] = '\0';
  }
  else
  {
    strcpy(appkey, app_key);
  }

  UserInfo user_info;

  RcClientImpl impl;
  impl.set_kv_rs_addr(krs_addr);
  int ret = impl.initialize(rc_addr, appkey, app_ip);

  if (TFS_SUCCESS != ret)
  {
    TBSYS_LOG(DEBUG, "rc client init failed, ret: %d", ret);
  }
  else
  {
    ret = impl.get_object(bucket_name, object_name, local_file, user_info);
  }
  ToolUtil::print_info(ret, "get object: %s, object: %s => %s",
      bucket_name, object_name, local_file);

  return ret;
}

int cmd_del_object(const VSTRING& param)
{
  const char* bucket_name = param[0].c_str();
  const char* object_name = param[1].c_str();
  char appkey[257];
  int size = param.size();
  if (size > 2)
  {
    strncpy(appkey, param[2].c_str(), 256);
    appkey[256] = '\0';
  }
  else
  {
    strcpy(appkey, app_key);
  }

  UserInfo user_info;

  RcClientImpl impl;
  impl.set_kv_rs_addr(krs_addr);
  int ret = impl.initialize(rc_addr, appkey, app_ip);

  if (TFS_SUCCESS != ret)
  {
    TBSYS_LOG(DEBUG, "rc client init failed, ret: %d", ret);
  }
  else
  {
    ret = impl.del_object(bucket_name, object_name, user_info);
  }
  ToolUtil::print_info(ret, "del bucket: %s, object: %s", bucket_name, object_name);

  return ret;
}

int cmd_head_object(const VSTRING& param)
{
  const char* bucket_name = param[0].c_str();
  const char* object_name = param[1].c_str();
  char appkey[257];
  int size = param.size();
  if (size > 2)
  {
    strncpy(appkey, param[2].c_str(), 256);
    appkey[256] = '\0';
  }
  else
  {
    strcpy(appkey, app_key);
  }

  ObjectInfo object_info;
  UserInfo user_info;

  RcClientImpl impl;
  impl.set_kv_rs_addr(krs_addr);
  int ret = impl.initialize(rc_addr, appkey, app_ip);

  if (TFS_SUCCESS != ret)
  {
    TBSYS_LOG(DEBUG, "rc client init failed, ret: %d", ret);
  }
  else
  {
    ret = impl.head_object(bucket_name, object_name, &object_info, user_info);
  }

  if (TFS_SUCCESS == ret)
  {
    object_info.dump();
  }
  ToolUtil::print_info(ret, "head bucket: %s, object: %s", bucket_name, object_name);

  return ret;
}

int cmd_set_life_cycle(const VSTRING& param)
{
  int32_t file_type = atoi(param[0].c_str());
  const char* file_name = param[1].c_str();
  int32_t invalid_time_s = atoi(param[2].c_str());
  const char* user_appkey = param[3].c_str();
  char appkey[257];

  strcpy(appkey, app_key);

  RcClientImpl impl;
  impl.set_kv_rs_addr(krs_addr);
  int ret = impl.initialize(rc_addr, appkey, app_ip);
  if (TFS_SUCCESS != ret)
  {
    TBSYS_LOG(DEBUG, "rc client init failed, ret: %d", ret);
  }
  else
  {
    ret = impl.set_life_cycle(file_type, file_name, invalid_time_s, user_appkey);
    TBSYS_LOG(DEBUG, "set_life_cycle return ret: %d", ret);
  }
  if (TFS_SUCCESS == ret)
  {
    ToolUtil::print_info(ret, "set life cycle file %s invalid_time_s : %d", file_name, invalid_time_s);
  }
  return ret;
}

int cmd_get_life_cycle(const VSTRING& param)
{
  int32_t file_type = atoi(param[0].c_str());
  const char* file_name = param[1].c_str();

  char appkey[257];

  strcpy(appkey, app_key);

  RcClientImpl impl;
  impl.set_kv_rs_addr(krs_addr);
  int ret = impl.initialize(rc_addr, appkey, app_ip);
  int32_t invalid_time_s = 0;
  if (TFS_SUCCESS != ret)
  {
    TBSYS_LOG(DEBUG, "rc client init failed, ret: %d", ret);
  }
  else
  {
    ret = impl.get_life_cycle(file_type, file_name, &invalid_time_s);
  }
  if (TFS_SUCCESS == ret)
  {
    ToolUtil::print_info(ret, "get life cycle file %s invalid_time_s : %d", file_name, invalid_time_s);
  }
  return ret;
}

int cmd_del_life_cycle(const VSTRING& param)
{
  int32_t file_type = atoi(param[0].c_str());
  const char* file_name = param[1].c_str();

  char appkey[257];

  strcpy(appkey, app_key);

  RcClientImpl impl;
  impl.set_kv_rs_addr(krs_addr);
  int ret = impl.initialize(rc_addr, appkey, app_ip);
  if (TFS_SUCCESS != ret)
  {
    TBSYS_LOG(DEBUG, "rc client init failed, ret: %d", ret);
  }
  else
  {
    ret = impl.rm_life_cycle(file_type, file_name);
  }
  if (TFS_SUCCESS == ret)
  {
    ToolUtil::print_info(ret, "del life cycle file %s success", file_name);
  }
  return ret;
}

int cmd_query_task(const VSTRING& param)
{
  int ret = TFS_SUCCESS;

  const char *es_ip = NULL;
  uint64_t es_id;
  es_ip = param[0].c_str();
  es_id = Func::get_host_ip(es_ip);
  /*
  char appkey[257];
  strcpy(appkey, app_key);

  vector<ServerExpireTask> res_task;

  RcClientImpl impl;
  impl.set_lifecycle_rs_addr(lrs_addr);
  ret = impl.initialize(rc_addr, appkey, app_ip);
  if (TFS_SUCCESS != ret)
  {
    TBSYS_LOG(DEBUG, "rc client init failed, ret: %d", ret);
  }
  else
  {
    ret = impl.query_task(es_id, &res_task);
  }
  if (TFS_SUCCESS == ret)
  {
    vector<ServerExpireTask>::iterator iter = res_task.begin();
    for (; iter != res_task.end(); ++iter)
    {
      cout << "expire_server : " << Func::addr_to_str(iter->server_id_, true) << " "
           << "alive_task_total : " << iter->task_.alive_total_ << " "
           << "this task num : " << iter->task_.assign_no_ << " "
           << "root start task_time : " << iter->task_.spec_time_ << " "
           << "task status : " << iter->task_.status_ << endl;
    }
  }
  ToolUtil::print_info(ret, "query task success");
*/
  return ret;
}

