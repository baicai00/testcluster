#include "Shop.h"
#include "common.h"
#include "pb/inner.pb.h"
#include <string>
#include <arpa/inet.h>
#include <stdlib.h>
using namespace std;


Shop::Shop()
{
    service_register_init_func(boost::bind(&Shop::shop_init, this, _1));
}

bool Shop::shop_init(const std::string& parm)
{
    sscanf(parm.c_str(), "%u", &m_cservice_proxy);

    register_callback();

    return true;
}

void Shop::service_start()
{

}

void Shop::register_callback()
{
    service_callback(pb::iTestPingShopREQ::descriptor()->full_name(), boost::bind(&Shop::proto_test_ping_shop, this, _1, _2));
}

void Shop::text_message(const void * msg, size_t sz, uint32_t source, int session)
{
    char* c = (char*)msg;
    const char* from = strsep(&c, " ");

    if (strcmp(from, "lua") == 0) {
        const char* uid = strsep(&c, " ");
        (void)uid;
        TextParm parm(c);
        const char* cmd = parm.get("cmd");
        string response;
        if (strcmp(cmd, "cmd_ping") == 0) {
            response = "pong " + to_string(time(NULL));
        }

        if (session != 0) {
            text_response(response);
        }
    }
}

void Shop::proto_test_ping_shop(Message* data, uint32_t source)
{
    pb::iTestPingShopREQ* msg = dynamic_cast<pb::iTestPingShopREQ*>(data);
    LOG(INFO) << "proto_test_ping_shop msg:" << msg->ShortDebugString();

    std::string remote_node = "activity_master_5";
    std::string remote_service = "activity_master";

    // test1,无返回消息
    pb::iTestPingActivityMsg req;
    req.set_ping_msg("Hello activity, I am shop, I use send");
    service_send_cluster(req, remote_node, remote_service);

    // test2,rpc_all
    pb::iTestPingActivityREQ rpc_req;
    rpc_req.set_ping_msg("Hello activity, I am shop, I use rpc call");
    rpc_call(m_cservice_proxy, rpc_req, [this] (Message* data) mutable {
        pb::iTestPingActivityRSP * rsp = dynamic_cast<pb::iTestPingActivityRSP*>(data);
        LOG(INFO) << "iTestPingActivityRSP:" << rsp->ShortDebugString();
    }, m_process_uid, remote_node, remote_service, "shop_master_4", "shop_master");
}

void Shop::service_send_cluster(const Message& msg, const string& remote_node, const string& remote_service)
{
    char* data;
    uint32_t size;
    int32_t roomid = get_service_roomid();
    uint32_t source = 0;
    int64_t uid = m_process_uid;
    uint32_t type = SUBTYPE_PROTOBUF;
    int32_t session = 0;
    serialize_imsg_proxy(msg, data, size, uid, type, roomid, session, remote_node, remote_service, "shop_master_4", "shop_master");
    skynet_send_noleak(m_ctx, source, m_cservice_proxy, PTYPE_TEXT | PTYPE_TAG_DONTCOPY, 0, data, size);
}