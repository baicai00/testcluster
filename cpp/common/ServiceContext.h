/*
 * ServiceContext用来存储公共数据结构, 每个service里面唯一实例
 *
 */

#ifndef __SERVICE_CONTEXT_H__
#define __SERVICE_CONTEXT_H__

#include "Pack.h"
#include <string>
#include <limits>
#include "../log/Log.h"
#include <unordered_map>
#include <sys/time.h>
#include "skynet_wrapper.h"

extern "C"
{
#include "skynet_env.h"
}

class skynet_context;
class ServiceContext final
{
    public:
        ServiceContext()
            : m_ctx(nullptr),
              m_pack_context_id(0),
              m_harborid(0),
              m_process_roomid(0)
        {
            m_harborid = atoi(skynet_getenv("harbor"));
            memset(m_xxtea_key, 0, sizeof(m_xxtea_key));
        }
        ServiceContext(const ServiceContext&) = delete;
        ServiceContext& operator= (const ServiceContext&) = delete;
        ~ServiceContext() = default;

        void init_service_context(skynet_context* ctx)
        {
            m_ctx = ctx;
        }

    public:

        void workload_monitor(const std::string& type_name, int64_t spend_us, uint64_t now_tick)
        {
        }

        void workload_monitor(const std::string& service_name, int32_t harborid, const std::string& type_name, int64_t spend_us, uint64_t now_tick)
        {
        }

    public:
        void set_profile(bool profile)
        {
        }
        bool profile() const
        {
            return false;
        }

        void set_profile_log_interval(uint64_t interval)
        {
        }

        void set_discard_msg(bool discard, uint64_t timeout)
        {
        }

        bool msg_discard() const
        {
            return false;
        }

        uint64_t msg_discard_timeout() const
        {
            return false;
        }

        void set_rsp_cache_switch(bool cache_switch)
        {
        }

        bool rsp_cache_switch() const
        {
            return false;
        }

    public:
        skynet_context* get_skynet_context() const
        {
            return m_ctx;
        }

        int64_t get_current_process_roomid() const
        {
            return m_process_roomid;
        }

        void set_current_process_roomid(int roomid)
        {
            m_process_roomid = roomid;
        }

        int32_t harborid() const
        {
            return m_harborid;
        }

        void service_over_event()
        {
        }


    private:
        uint16_t get_pack_context_id()
        {
            if (m_pack_context_id >= std::numeric_limits<uint16_t>::max())
            {
                m_pack_context_id = 0;
            }
            else
            {
                ++m_pack_context_id;
            }
            return m_pack_context_id;
        }

    private:
        skynet_context* m_ctx;
        uint16_t    m_pack_context_id;      // 用于给PackContext的自增
        int32_t     m_harborid;
        char m_xxtea_key[16];
        int m_process_roomid; // 包头roomid
};

#endif
