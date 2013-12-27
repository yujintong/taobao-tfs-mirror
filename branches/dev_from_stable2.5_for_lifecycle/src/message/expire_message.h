/*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: $
 *
 * Authors:
 *   qixiao.zs <qixiao.zs@alibaba-inc.com>
 *      - initial release
 *
 */
#ifndef TFS_MESSAGE_EXPIRE_MESSAGE_H_
#define TFS_MESSAGE_EXPIRE_MESSAGE_H_

#include "common/expire_define.h"
#include "common/base_packet.h"

namespace tfs
{
  namespace message
  {
    class ReqCleanTaskFromRtsMessage : public common::BasePacket
    {
      public:
        ReqCleanTaskFromRtsMessage();
        virtual ~ReqCleanTaskFromRtsMessage();
        virtual int serialize(common::Stream& output) const;
        virtual int deserialize(common::Stream& input);
        virtual int64_t length() const;
        common::ExpireTaskInfo get_task() const
        {
          return task_;
        }
        void set_task(const common::ExpireTaskInfo& task)
        {
          task_ = task;
        }

      private:
        common::ExpireTaskInfo task_;
    };

    class ReqFinishTaskFromEsMessage : public common::BasePacket
    {
      public:
        ReqFinishTaskFromEsMessage();
        virtual ~ReqFinishTaskFromEsMessage();
        virtual int serialize(common::Stream& output) const;
        virtual int deserialize(common::Stream& input);
        virtual int64_t length() const;

        uint64_t get_es_id() const
        {
          return es_id_;
        }

        void set_es_id(const uint64_t es_id)
        {
          es_id_ = es_id;
        }
        void set_task(const common::ExpireTaskInfo& rh)
        {
          task_ = rh;
        }
        common::ExpireTaskInfo get_task() const
        {
          return task_;
        }

      private:
        uint64_t es_id_;
        common::ExpireTaskInfo task_;
    };

    class ReqRtsEsHeartMessage: public common::BasePacket
    {
      public:
        ReqRtsEsHeartMessage();
        virtual ~ReqRtsEsHeartMessage();

        virtual int serialize(common::Stream& output) const ;
        virtual int deserialize(common::Stream& input);
        virtual int64_t length() const;
        inline common::ExpServerBaseInformation& get_mutable_es(void) { return base_info_;}
        inline void set_es(const common::ExpServerBaseInformation& base_info){ memcpy(&base_info_, &base_info, sizeof(common::ExpServerBaseInformation));}

      private:
        common::ExpServerBaseInformation base_info_;
    };

    class RspRtsEsHeartMessage: public common::BasePacket
    {
      public:
        RspRtsEsHeartMessage();
        virtual ~RspRtsEsHeartMessage();

        virtual int serialize(common::Stream& output) const ;
        virtual int deserialize(common::Stream& input);
        virtual int64_t length() const;
        inline int32_t get_time(void) { return heart_interval_;}
        inline void set_time(const int32_t heart_interval){ heart_interval_ = heart_interval;}

      private:
        int32_t heart_interval_;
    };

    class ReqQueryProgressMessage: public common::BasePacket
    {
      public:
        ReqQueryProgressMessage();
        virtual ~ReqQueryProgressMessage();
        virtual int serialize(common::Stream &output) const;
        virtual int deserialize(common::Stream &input);
        virtual int64_t length() const;

        inline uint64_t get_es_id() const {return es_id_;}
        inline void set_es_id(const uint64_t es_id) {es_id_ = es_id;}

        inline int32_t get_es_num() const {return es_num_;}
        inline void set_es_num(const int32_t es_num) {es_num_ = es_num;}

        inline int32_t get_task_time() const {return task_time_;}
        inline void set_task_time(const int32_t task_time) {task_time_ = task_time;}

        inline int32_t get_hash_bucket_id() const {return hash_bucket_id_;}
        inline void set_hash_bucket_id(const int32_t hash_bucket_id) {hash_bucket_id_ = hash_bucket_id;}

        inline common::ExpireTaskInfo::ExpireTaskType get_expire_task_type() const {return type_;}
        inline void set_expire_task_type(common::ExpireTaskInfo::ExpireTaskType &type){type_ = type;}

      private:
        uint64_t es_id_;
        int32_t es_num_;
        int32_t task_time_;
        int32_t hash_bucket_id_;
        common::ExpireTaskInfo::ExpireTaskType type_;
    };

    class RspQueryProgressMessage: public common::BasePacket
    {
      public:
        RspQueryProgressMessage();
        virtual ~RspQueryProgressMessage();
        virtual int serialize(common::Stream &output) const;
        virtual int deserialize(common::Stream &input);
        virtual int64_t length() const;

        inline int32_t get_sum_file_num() const {return sum_file_num_;}
        inline void set_sum_file_num(const int32_t sum_file_num) {sum_file_num_ = sum_file_num;}

        inline int32_t get_current_percent() const {return current_percent_;}
        inline void set_current_percent(const int32_t current_percent) {current_percent_ = current_percent;}

      private:
        int32_t sum_file_num_;
        int32_t current_percent_;
    };

  }
}
#endif
