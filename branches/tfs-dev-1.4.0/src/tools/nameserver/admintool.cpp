#include <stdio.h>
#include <pthread.h>
#include <signal.h>

#include <vector>
#include <string>
#include <map>

#include "tbsys.h"

#include "common/internal.h"
#include "common/client_manager.h"
#include "common/config_item.h"
#include "common/status_message.h"
#include "common/new_client.h"
#include "message/server_status_message.h"
#include "message/client_cmd_message.h"
#include "message/message_factory.h"
#include "new_client/fsname.h"
#include "new_client/tfs_client_api.h"
#include "tools/util/tool_util.h"

using namespace tfs::common;
using namespace tfs::message;
using namespace tfs::client;
using namespace tfs::tools;
using namespace std;

static TfsClient* g_tfs_client = NULL;
static STR_FUNC_MAP g_cmd_map;
static uint64_t g_local_server_ip = 0;

int usage(const char *name);
static void sign_handler(const int32_t sig);
int main_loop();
int do_cmd(char* buf);
void init();
int get_file_retry(char* tfs_name, char* local_file);

/* cmd func */
int cmd_show_help(const VSTRING&);
int cmd_quit(const VSTRING&);
int cmd_set_run_param(const VSTRING& param);
int cmd_add_block(const VSTRING& param);
int cmd_remove_block(const VSTRING& param);
int cmd_expire_block(const VSTRING& param);
int cmd_unexpire_block(const VSTRING& param);
int cmd_compact_block(const VSTRING& param);
int cmd_repair_lose_block(const VSTRING& param);
int cmd_repair_group_block(const VSTRING& param);
int cmd_repair_crc(const VSTRING& param);
int cmd_access_stat_info(const VSTRING& param);
int cmd_access_control_flag(const VSTRING& param);
int cmd_rotate_log(const VSTRING& param);
int cmd_dump_plan(const VSTRING &param);

#ifdef _WITH_READ_LINE
#include "readline/readline.h"
#include "readline/history.h"
template<class T> const char* get_str(T it)
{
  return it->first.c_str();
}

template<> const char* get_str(VSTRING::iterator it)
{
  return (*it).c_str();
}

template<class T> char* do_match(const char* text, int state, T& m)
{
  static typename T::iterator it;
  static int len = 0;
  const char* cmd = NULL;

  if (!state)
  {
    it = m.begin();
    len = strlen(text);
  }

  while(it != m.end())
  {
    cmd = get_str(it);
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

char* match_cmd(const char* text, int32_t state)
{
  return do_match(text, state, g_cmd_map);
}

char** admin_cmd_completion (const char* text, int, int)
{
  // disable default filename completion
  rl_attempted_completion_over = 1;
  return rl_completion_matches(text, match_cmd);
}
#endif

void init()
{
  g_cmd_map["help"] = CmdNode("help", "show help info", 0, 0, cmd_show_help);
  g_cmd_map["quit"] = CmdNode("quit", "quit", 0, 0, cmd_quit);
  g_cmd_map["exit"] = CmdNode("exit", "exit", 0, 0, cmd_quit);
  g_cmd_map["param"] = CmdNode("param name [set value [extravalue]]", "set/get param value", 1, 4, cmd_set_run_param);
  g_cmd_map["removeblock"] = CmdNode("removeblock blockid", "remove block", 1, 1, cmd_remove_block);
  g_cmd_map["expblk"] = CmdNode("expblk blockid dsip:port", "expire block", 2, 2, cmd_expire_block);
  g_cmd_map["ueblk"] = CmdNode("ueblk blockid dsip:port", "unexpire block", 2, 2, cmd_unexpire_block);
  g_cmd_map["compact"] = CmdNode("compact blockid", "compact block", 1, 1, cmd_compact_block);
  g_cmd_map["repairblk"] = CmdNode("repairblk blockid [src dest action]", "repair block", 1, 4, cmd_repair_lose_block);
  g_cmd_map["repairgrp"] = CmdNode("repairgrp blockid", "repairgrp block", 1, 1, cmd_repair_group_block);
  g_cmd_map["repaircrc"] = CmdNode("repaircrc filename", "repair file's crc", 1, 1, cmd_repair_crc);
  g_cmd_map["aci"] = CmdNode("aci ip:port [startrow returnrow]", "access control", 1, 3, cmd_access_stat_info);
  g_cmd_map["setacl"] = CmdNode("setacl ip:port type [v1 [v2]]","set access control", 1, 4, cmd_access_control_flag);
  g_cmd_map["rotatelog"] = CmdNode("rotatelog ip:port","rotate log", 1, 1, cmd_rotate_log);
  g_cmd_map["dumpplan"] = CmdNode("dumpplan [serverip:port [action]]", "dump plan server", 0, 2, cmd_dump_plan);
}

int cmd_set_run_param(const VSTRING& param)
{
  const static char* param_str[] = {
    "min_replication",
    "max_replication",
    "max_write_file_count",
    "max_use_capacity_ratio",
    "heart_interval",
    "replicate_wait_time",
    "compact_delete_ratio",
    "compact_max_load",
    "plan_run_flag",
    "run_plan_expire_interval",
    "run_plan_ratio",
    "object_dead_max_time",
    "balance_max_diff_block_num",
    "log_level",
    "add_primary_block_count",
    "build_plan_interval",
    "replicate_ratio",
    "max_wait_write_lease",
    "cluster_index",
    "build_plan_default_wait_time"
  };
  static int32_t param_strlen = sizeof(param_str) / sizeof(char*);

  int32_t i;
  int32_t size = param.size();
  if (size != 1 && size != 3 && size != 4)
  {
    fprintf(stderr, "param param_name\n\n");
    for (i = 0; i < param_strlen; i++)
    {
      fprintf(stderr, "%s\n", param_str[i]);
    }
    return TFS_ERROR;
  }

  const char* param_name = param[0].c_str();
  uint32_t index = 0;
  for (i = 0; i < param_strlen; i++)
  {
    if (strcmp(param_name, param_str[i]) == 0)
    {
      index = i + 1;
      break;
    }
  }
  if (0 == index)
  {
    fprintf(stderr, "param %s not valid\n", param_name);
    return TFS_ERROR;
  }
  uint64_t value = 0;
  if (3 == size || 4 == size)
  {
    if (strcmp("set", param[1].c_str()))
    {
      fprintf(stderr, "param %s set value\n\n", param_name);
      return TFS_ERROR;
    }
    index |= 0x10000000;
    value = atoi(param[2].c_str());
    if (4 == size)
    {
      value <<= 32;
      value |= atoi(param[3].c_str());
    }
  }

  ClientCmdMessage req_cc_msg;
  req_cc_msg.set_cmd(CLIENT_CMD_SET_PARAM);
  req_cc_msg.set_value3(index);
  req_cc_msg.set_value1(value);

  int32_t status = TFS_ERROR;

  send_msg_to_server(g_tfs_client->get_server_id(), &req_cc_msg, status);

  ToolUtil::print_info(status, "param %s %s %s", param[0].c_str(), index & 0x10000000 ? "set" : "",
                       index & 0x10000000 ? param[2].c_str() : "");

  return status;
}

int cmd_remove_block(const VSTRING&)
{
/*  int32_t size = param.size();
    if (size != 1)
    {
    fprintf(stderr, "removeblock block_id\n\n");
    return TFS_ERROR;
    }
    uint32_t block_id = strtoul(param[0].c_str(), reinterpret_cast<char**> (NULL), 10);
    VUINT64 ds_list;
    ds_list.clear();
    int32_t ret = tfs_client.get_block_info(block_id, ds_list);
    if (ret != TFS_SUCCESS)
    {
    fprintf(stderr, "block no exist in nameserver, blockid:%u.\n", block_id);
    return ret;
    }
    fprintf(stdout, "------block: %u, has %d replicas------\n", block_id, static_cast<int32_t> (ds_list.size()));
    //remove meta
    ds_list.push_back(0);
    for (uint32_t i = 0; i < ds_list.size(); ++i)
    {
    if (i < ds_list.size() - 1)
    {
    fprintf(stdout, "removeblock: %u, (%d)th server: %s \n", block_id, i,
    tbsys::CNetUtil::addrToString(ds_list[i]).c_str());
    }

    //uint64_t nsip_port = tfs_client.get_ns_ip_port();

    ClientCmdMessage req_cc_msg;
    req_cc_msg.set_cmd(CLIENT_CMD_EXPBLK);
    req_cc_msg.set_value1(ds_list[i]);
    req_cc_msg.set_value3(block_id);
    req_cc_msg.set_value4(0);
    req_cc_msg.set_value2(g_local_server_ip);
    string err_msg;
    NewClient* client = NewClientManager::get_instance().create_client();
    if (send_message_to_server(ns_id, client, &req_cc_msg, err_msg) == TFS_ERROR)
    {
    fprintf(stderr, "%s\n\n", err_msg.c_str());
    NewClientManager::get_instance().destroy_client(client);
    return TFS_ERROR;
    }
    NewClientManager::get_instance().destroy_client(client);
    }
*/  return TFS_SUCCESS;
}

int cmd_expire_block(const VSTRING& param)
{
  uint32_t block_id = atoi(param[0].c_str());
  uint64_t server_id = Func::get_host_ip(param[1].c_str());
  if (0 == server_id || 0 == block_id)
  {
    fprintf(stderr, "invalid address or blockid: %s %s\n", param[0].c_str(), param[1].c_str());
    return TFS_ERROR;
  }

  ClientCmdMessage req_cc_msg;
  req_cc_msg.set_cmd(CLIENT_CMD_EXPBLK);
  req_cc_msg.set_value1(server_id);
  req_cc_msg.set_value3(block_id);
  req_cc_msg.set_value4(0);
  req_cc_msg.set_value2(g_local_server_ip);

  int32_t status = TFS_ERROR;

  send_msg_to_server(g_tfs_client->get_server_id(), &req_cc_msg, status);

  ToolUtil::print_info(status, "expireblock %s %s", param[0].c_str(), param[1].c_str());

  return status;
}

int cmd_unexpire_block(const VSTRING& param)
{
  uint32_t block_id = atoi(param[0].c_str());
  uint64_t server_id = Func::get_host_ip(param[1].c_str());

  if (0 == server_id || 0 == block_id)
  {
    fprintf(stderr, "invalid blockid or address: %s %s\n", param[0].c_str(), param[1].c_str());
    return TFS_ERROR;
  }

  ClientCmdMessage req_cc_msg;
  req_cc_msg.set_cmd(CLIENT_CMD_LOADBLK);
  req_cc_msg.set_value1(server_id);
  req_cc_msg.set_value3(block_id);
  req_cc_msg.set_value4(0);
  req_cc_msg.set_value2(g_local_server_ip);

  int status = TFS_ERROR;

  send_msg_to_server(g_tfs_client->get_server_id(), &req_cc_msg, status);

  ToolUtil::print_info(status, "unexpireblock %s %s", param[0].c_str(), param[1].c_str());

  return status;
}

int cmd_compact_block(const VSTRING& param)
{
  uint32_t block_id = atoi(param[0].c_str());
  if (block_id <= 0)
  {
    fprintf(stderr, "invalid block id: %u\n", block_id);
    return TFS_ERROR;
  }

  ClientCmdMessage req_cc_msg;
  req_cc_msg.set_cmd(CLIENT_CMD_COMPACT);
  req_cc_msg.set_value1(0);
  req_cc_msg.set_value3(block_id);
  req_cc_msg.set_value4(0);
  req_cc_msg.set_value2(g_local_server_ip);

  int32_t status = TFS_ERROR;
  send_msg_to_server(g_tfs_client->get_server_id(), &req_cc_msg, status);
  ToolUtil::print_info(status, "compactblock %u", block_id);

  return status;
}

int cmd_repair_lose_block(const VSTRING& param)
{
  int32_t size = param.size();
  int32_t action = 2;
  if (size != 1 && size != 4)
  {
    fprintf(stderr, "repairblk block_id [source] [dest] [action]\n\n");
    return TFS_ERROR;
  }
  uint32_t block_id = atoi(param[0].c_str());
  if (block_id <= 0)
  {
    fprintf(stderr, "invalid blockid: %u\n", block_id);
    return TFS_ERROR;
  }

  uint64_t src_ns_id = 0;
  uint64_t dest_ns_id = 0;
  if (4 == size)
  {
    src_ns_id = Func::get_host_ip(param[1].c_str());
    dest_ns_id = Func::get_host_ip(param[2].c_str());
    action = atoi(param[3].c_str());
  }

  ClientCmdMessage req_cc_msg;
  req_cc_msg.set_cmd(CLIENT_CMD_IMMEDIATELY_REPL);
  req_cc_msg.set_value3(block_id);
  req_cc_msg.set_value4(action);
  req_cc_msg.set_value2(src_ns_id);
  req_cc_msg.set_value1(dest_ns_id);

  int32_t status = TFS_ERROR;
  send_msg_to_server(g_tfs_client->get_server_id(), &req_cc_msg, status);
  ToolUtil::print_info(status, "repairblk %u %"PRI64_PREFIX"u %"PRI64_PREFIX"u", block_id, src_ns_id, dest_ns_id);

  return status;
}

int cmd_repair_group_block(const VSTRING& param)
{
  uint32_t block_id = atoi(param[0].c_str());

  if (block_id <= 0)
  {
    fprintf(stderr, "invalid block id %u\n", block_id);
    return TFS_ERROR;
  }

    ClientCmdMessage req_cc_msg;
    req_cc_msg.set_cmd(CLIENT_CMD_REPAIR_GROUP);
    req_cc_msg.set_value3(block_id);
    req_cc_msg.set_value4(0);
    req_cc_msg.set_value1(g_local_server_ip);

    int32_t status = TFS_ERROR;
    send_msg_to_server(g_tfs_client->get_server_id(), &req_cc_msg, status);
    ToolUtil::print_info(status, "repairgrp %u", block_id);

    return status;
}

int cmd_repair_crc(const VSTRING&)
{
/*  int32_t size = param.size();
    if (size != 1)
    {
    fprintf(stderr, "repaircrc filename\n\n");
    return TFS_ERROR;
    }

    char* tfs_name = const_cast<char*> (param[0].c_str());
    if (static_cast<int32_t> (strlen(tfs_name)) < FILE_NAME_LEN || tfs_name[0] != 'T')
    {
    fprintf(stderr, "file name error: %s\n", tfs_name);
    return TFS_ERROR;
    }

    char local_file[56];
    sprintf(local_file, ".repair_crc_%s", tfs_name);
    int32_t ret = get_file_retry(tfs_client, tfs_name, local_file);
    if (ret == TFS_SUCCESS)
    {
    ret = tfs_client.save_file(local_file, tfs_name, NULL);
    if (ret)
    {
    fprintf(stderr, "save failed: %s => %s\n", local_file, tfs_name);
    }
    }
    else if (ret == TFS_ERROR - 100)
    {
    fprintf(stderr, "file not exits: %s\n", tfs_name);
    }
    else
    {
    fprintf(stderr, "don't have such file: %s\n", tfs_name);
    }
    unlink(local_file);

    tfs_client.tfs_close();
    return ret;
*/
  return 0;
}

int cmd_access_stat_info(const VSTRING& param)
{
  int32_t size = param.size();
  uint64_t server_id = Func::get_host_ip(param[0].c_str());

  uint32_t start_row = 0;
  uint32_t return_row = 0;

  if (size > 1)
  {
    start_row = atoi(param[1].c_str());
  }
  if (size > 2)
  {
    return_row = atoi(param[2].c_str());
  }

  bool get_all = (start_row == 0 && return_row == 0);
  if (get_all)
  {
    return_row = 1000;
  }
  int32_t has_next = 0;

  GetServerStatusMessage req_gss_msg;

  fprintf(stdout,
          "ip addr           | read count  | read bytes  | write count  | write bytes\n"
          "------------------ -------------- ------------- -------------- ------------\n");

  int ret = TFS_SUCCESS;
  while (1)
  {
    req_gss_msg.set_status_type(GSS_CLIENT_ACCESS_INFO);
    req_gss_msg.set_from_row(start_row);
    req_gss_msg.set_return_row(return_row);

    tbnet::Packet* ret_message = NULL;
    NewClient* client = NewClientManager::get_instance().create_client();
    send_msg_to_server(server_id, client, &req_gss_msg, ret_message);

    if (ret_message == NULL)
    {
      ret = TFS_ERROR;
    }
    else if (ret_message->getPCode() == ACCESS_STAT_INFO_MESSAGE)
    {
      AccessStatInfoMessage* req_cb_msg = reinterpret_cast<AccessStatInfoMessage*> (ret_message);
      const AccessStatInfoMessage::COUNTER_TYPE & m = req_cb_msg->get();
      for (AccessStatInfoMessage::COUNTER_TYPE::const_iterator it = m.begin(); it != m.end(); ++it)
      {
        printf("%15s : %14" PRI64_PREFIX "u %14s %14" PRI64_PREFIX "u %14s\n",
               tbsys::CNetUtil::addrToString(it->first).c_str(),
               it->second.read_file_count_, Func::format_size(it->second.read_byte_).c_str(),
               it->second.write_file_count_, Func::format_size(it->second.write_byte_).c_str());
      }

      has_next = req_cb_msg->has_next();
    }
    else if (ret_message->getPCode() == STATUS_MESSAGE)
    {
      StatusMessage* s_msg = dynamic_cast<StatusMessage*> (ret_message);
      fprintf(stderr, "get status msg, ret: %d, error: %s\n", s_msg->get_status(), s_msg->get_error());
      ret = s_msg->get_status();
    }

    NewClientManager::get_instance().destroy_client(client);

    if (TFS_SUCCESS == ret)
    {
      if (get_all)
      {
        if (!has_next)
        {
          break;
        }
        else
        {
          start_row += return_row;
        }
      }
      else
      {
        break;
      }
    }
    else
    {
      break;
    }
  }
  return ret;
}

int cmd_access_control_flag(const VSTRING& param)
{
  int32_t size = param.size();
  uint64_t server_id = Func::get_host_ip(param[0].c_str());
  uint32_t op_type = atoi(param[1].c_str());
  if (op_type < 1 || op_type > 5)
  {
    fprintf(stderr, "error type %d must in [1,5]\n\n", op_type);
    return TFS_ERROR;
  }

  const char* value1 = NULL;
  const char* value2 = NULL;
  uint64_t v1 = 0;
  uint32_t v2 = 0;
  if (size > 2)
  {
    value1 = param[2].c_str();
  }

  if (size > 3)
  {
    value2 = param[3].c_str();
  }

  switch (op_type)
  {
  case 1:
    if (!value1)
    {
      fprintf(stderr, "setacl ip:port 1 flag\n");
      return TFS_ERROR;
    }
    v1 = atoi(value1);
    v2 = 0;
    break;
  case 2:
    if (!value1 || !value2)
    {
      fprintf(stderr, "setacl ip:port 2 ip mask\n");
      return TFS_ERROR;
    }
    v1 = tbsys::CNetUtil::strToAddr(const_cast<char*> (value1), 0);
    v2 = static_cast<uint32_t> (tbsys::CNetUtil::strToAddr(const_cast<char*> (value2), 0));
    if (!v1 || !v2)
    {
      fprintf(stderr, "setacl ip:port 2 ip mask, not  a valid ip & mask\n");
      return TFS_ERROR;
    }
    break;
  case 3:
    if (!value1)
    {
      fprintf(stderr, "setacl ip:port 3 ipaddr\n");
      return TFS_ERROR;
    }
    v1 = tbsys::CNetUtil::strToAddr(const_cast<char*> (value1), 0);
    v2 = 0;
    break;
  case 4:
  case 5:
    v1 = 0;
    v2 = 0;
    break;
  default:
    fprintf(stderr, "error type %d must in [1,5]\n\n", op_type);
    return TFS_ERROR;
  }

  ClientCmdMessage req_cc_msg;
  req_cc_msg.set_cmd(CLIENT_CMD_SET_PARAM);
  req_cc_msg.set_value3(op_type); // param type == 1 as set acl flag.
  req_cc_msg.set_value1(v1); // ns_id as flag
  req_cc_msg.set_value4(v2);
  req_cc_msg.set_value2(g_local_server_ip);

  int32_t status = TFS_ERROR;
  send_msg_to_server(server_id, &req_cc_msg, status);
  ToolUtil::print_info(status, "set acl %s", param[0].c_str());
  return status;
}

int cmd_rotate_log(const VSTRING& param)
{
  uint64_t server_id = Func::get_host_ip(param[0].c_str());
  ClientCmdMessage req_cc_msg;
  req_cc_msg.set_cmd(CLIENT_CMD_ROTATE_LOG);
  int32_t status = TFS_ERROR;

  send_msg_to_server(server_id, &req_cc_msg, status);
  ToolUtil::print_info(status, "rotatelog %s", param[0].c_str());
  return status;
}

int cmd_show_help(const VSTRING&)
{
  return ToolUtil::show_help(g_cmd_map);
}

int cmd_quit(const VSTRING&)
{
  return TFS_CLIENT_QUIT;
}

int cmd_get_file_retry(char*, char*)
{
//   fprintf(stderr, "filename: %s\n", tfs_name);
//   fflush( stderr);
//   int tfs_fd = 0;
//   tfs_fd = g_tfs_client->open(tfs_name, NULL, T_READ);
//   if (tfs_fd < 0)
//   {
//     fprintf(stderr, "open tfs_client fail\n");
//     return TFS_ERROR;
//   }
//   TfsFileStat file_info;
//   if (g_tfs_client->fstat(tfs_fd, &file_info) == TFS_ERROR)
//   {
//     g_tfs_client->close(tfs_fd);
//     fprintf(stderr, "fstat tfs_client fail\n");
//     return TFS_ERROR;
//   }

//   int32_t done = 0;
//   while (done >= 0 && done <= 10)
//   {
//     int64_t t1 = Func::curr_time();
//     int32_t fd = open(local_file, O_WRONLY | O_CREAT | O_TRUNC, 0600);
//     if (-1 == fd)
//     {
//       fprintf(stderr, "open local file fail: %s\n", local_file);
//       g_tfs_client->close(tfs_fd);
//       return TFS_ERROR;
//     }

//     char data[MAX_READ_SIZE];
//     uint32_t crc = 0;
//     int32_t total_size = 0;
//     for (;;)
//     {
//       int32_t read_len = g_tfs_client->read(tfs_fd, data, MAX_READ_SIZE);
//       if (read_len < 0)
//       {
//         fprintf(stderr, "read tfs_client fail\n");
//         break;
//       }
//       if (0 == read_len)
//       {
//         break;
//       }
//       if (write(fd, data, read_len) != read_len)
//       {
//         fprintf(stderr, "write local file fail: %s\n", local_file);
//         g_tfs_client->close(tfs_fd);
//         close(fd);
//         return TFS_ERROR;
//       }
//       crc = Func::crc(crc, data, read_len);
//       total_size += read_len;
//       if (read_len != MAX_READ_SIZE)
//       {
//         break;
//       }
//     }
//     close(fd);
//     if (crc == file_info.crc_ && total_size == file_info.size_)
//     {
//       g_tfs_client->close(tfs_fd);
//       return TFS_SUCCESS;
//     }

//     int64_t t2 = Func::curr_time();
//     if (t2 - t1 > 500000)
//     {
//       //fprintf(stderr, "filename: %s, time: %" PRI64_PREFIX "d, server: %s, done: %d\n", tfs_name, t2 - t1,
//       //   tbsys::CNetUtil::addrToString(tfs_client->get_last_elect_ds_id()).c_str(), done);
//       fflush(stderr);
//     }
//     done++;
//     /*if (tfs_client->tfs_reset_read() <= done)
//       {
//       break;
//       }*/
//   }
//   tfs_client->close(tfs_fd);
  return TFS_SUCCESS;
}

int main(int argc,char** argv)
{
  int32_t i;
  bool directly = false;
  const char* dev_name = "eth0";
  const char* ns_ip = NULL;

  while ((i = getopt(argc, argv, "s:d:ih")) != EOF)
  {
    switch (i)
    {
    case 's':
      ns_ip = optarg;
      break;
    case 'd':
      dev_name = optarg;
      break;
    case 'i':
      directly = true;
      break;
    case 'h':
    default:
      usage(argv[0]);
    }
  }

  if (NULL == ns_ip)
  {
    fprintf(stderr, "please input nameserver ip and port.\n");
    usage(argv[0]);
  }

  g_tfs_client = TfsClient::Instance();
  int ret = g_tfs_client->initialize(ns_ip);
  if (ret != TFS_SUCCESS)
  {
    fprintf(stderr, "init tfs client fail. ret: %d\n", ret);
    return ret;
  }

  init();

  g_local_server_ip = tbsys::CNetUtil::getLocalAddr(dev_name);

  if (optind >= argc)
  {
    signal(SIGINT, sign_handler);
    signal(SIGTERM, sign_handler);
    main_loop();
  }
  else
  {
    if (directly)
    {
      for (i = optind; i < argc; i++)
      {
        do_cmd(argv[i]);
      }
    }
    else
    {
      usage(argv[0]);
    }
  }
}

int usage(const char *name)
{
  fprintf(stderr,
          "\n****************************************************************************** \n"
          "You can operate nameserver by this tool.\n"
          "Usage: \n"
          "  %s -s ns_ip_port [-d dev_name] [-i 'command'] [-h help]\n"
          "****************************************************************************** \n\n",
          name);
  exit(TFS_ERROR);
}

static void sign_handler(const int32_t sig)
{
  switch (sig)
  {
  case SIGINT:
  case SIGTERM:
    fprintf(stderr, "admintool exit.\n");
    exit(TFS_ERROR);
    break;
  default:
    break;
  }
}

inline bool is_whitespace(char c)
{
  return (' ' == c || '\t' == c);
}

inline char* strip_line(char* line)
{
  while (is_whitespace(*line))
  {
    line++;
  }
  int32_t end = strlen(line);
  while (end && (is_whitespace(line[end-1]) || '\n' == line[end-1] || '\r' == line[end-1]))
  {
    end--;
  }
  line[end] = '\0';
  return line;
}

int main_loop()
{
#ifdef _WITH_READ_LINE
  char* cmd_line = NULL;
  rl_attempted_completion_function = admin_cmd_completion;
#else
  char cmd_line[CMD_MAX_LEN];
#endif
  int ret = TFS_ERROR;
  while (1)
  {
    std::string tips = "";
    tips = "TFS > ";
#ifdef _WITH_READ_LINE
    cmd_line = readline(tips.c_str());
    if (!cmd_line)
#else
      fprintf(stderr, tips.c_str());

    if (NULL == fgets(cmd_line, CMD_MAX_LEN, stdin))
#endif
    {
      break;
    }
    ret = do_cmd(cmd_line);
#ifdef _WITH_READ_LINE

    delete cmd_line;
    cmd_line = NULL;
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
  key = strip_line(key);
  if (!key[0])
  {
    return TFS_SUCCESS;
  }
#ifdef _WITH_READ_LINE
  // not blank line, add to history
  add_history(key);
#endif

  char* token = strchr(key, ' ');
  if (token != NULL)
  {
    *token = '\0';
  }

  STR_FUNC_MAP_ITER it = g_cmd_map.find(Func::str_to_lower(key));

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
      break;
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

int cmd_dump_plan(const VSTRING& param)
{
  int32_t size = param.size();
  int32_t action = 0;
  uint64_t server_id = g_tfs_client->get_server_id();

  if (size >= 1)
  {
    server_id = Func::get_host_ip(param[0].c_str());
  }

  if (size == 2)
  {
    action = atoi(param[1].c_str());
  }

  DumpPlanMessage req_dp_msg;
  tbnet::Packet* ret_message = NULL;

  NewClient* client = NewClientManager::get_instance().create_client();

  int ret = send_msg_to_server(server_id, client, &req_dp_msg, ret_message);

  ToolUtil::print_info(ret, "%s", "dump plan");

  if (ret_message != NULL &&
      ret_message->getPCode() == DUMP_PLAN_RESPONSE_MESSAGE)
  {
    DumpPlanResponseMessage* req_dpr_msg = dynamic_cast<DumpPlanResponseMessage*>(ret_message);
    tbnet::DataBuffer& data_buff = req_dpr_msg->get_data();
    uint32_t plan_num = data_buff.readInt32();

    if (plan_num == 0)
    {
      printf("There is no plan currently.\n");
    }
    else
    {
      uint8_t plan_type;
      uint8_t plan_status;
      uint8_t plan_priority;
      uint32_t block_id;
      uint64_t plan_begin_time;
      uint64_t plan_end_time;
      uint64_t plan_seqno;
      uint8_t server_num;
      std::string runer;
      uint8_t plan_complete_status_num;
      std::vector< std::pair <uint64_t, uint8_t> > plan_compact_status_vec;
      std::pair<uint64_t, uint8_t> plan_compact_status_pair;

      printf("Plan Number(running + pending):%d\n", plan_num);
      printf("seqno   type       status     priority   block_id   begin        end           runer  \n");
      printf("------  ---------  -------    ---------  --------   -----------  ------------  -------\n");

      for (uint32_t i=0; i<plan_num; i++)
      {
        plan_type = data_buff.readInt8();
        plan_status = data_buff.readInt8();
        plan_priority = data_buff.readInt8();
        block_id = data_buff.readInt32();
        plan_begin_time = data_buff.readInt64();
        plan_end_time = data_buff.readInt64();
        plan_seqno = data_buff.readInt64();
        server_num = data_buff.readInt8();

        for (uint32_t j=0; j<server_num; j++)
        {
          runer += tbsys::CNetUtil::addrToString(data_buff.readInt64());
          runer += "/";
        }

        if (plan_type == PLAN_TYPE_COMPACT)
        {
          plan_complete_status_num = data_buff.readInt8();
          plan_complete_status_num = 0;
          plan_compact_status_vec.clear();
          for (uint32_t k=0; k<plan_complete_status_num; k++)
          {
            plan_compact_status_pair.first = data_buff.readInt64();
            plan_compact_status_pair.second = data_buff.readInt8();
            plan_compact_status_vec.push_back(plan_compact_status_pair);
          }
        }

        //display plan info
        printf("%-10"PRI64_PREFIX"d %-10s %-10s %-10s %-10u %-12"PRI64_PREFIX"d %-12"PRI64_PREFIX"d %-20s\n",
               plan_seqno,
               plan_type == PLAN_TYPE_REPLICATE ? "replicate" : plan_type == PLAN_TYPE_MOVE ? "move" : plan_type == PLAN_TYPE_COMPACT ? "compact" : plan_type == PLAN_TYPE_DELETE ? "delete" : "unknow",
               plan_status == PLAN_STATUS_BEGIN ? "begin" : plan_status == PLAN_STATUS_TIMEOUT ? "timeout" : plan_status == PLAN_STATUS_END ? "finish" : plan_status == PLAN_STATUS_FAILURE ? "failure": "unknow",
               plan_priority == PLAN_PRIORITY_NORMAL ? "normal" : plan_priority == PLAN_PRIORITY_EMERGENCY ? "emergency": "unknow",
               block_id, plan_begin_time, plan_end_time, runer.c_str());
      }
    }
  }
  else
  {
    fprintf(stderr, "invalid response message\n");
  }

  NewClientManager::get_instance().destroy_client(client);

  return ret;
}
