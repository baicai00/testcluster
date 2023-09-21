#include "Mail.h"
#include "common.h"
#include "pb/inner.pb.h"
#include <string>
#include <arpa/inet.h>
#include <stdlib.h>
using namespace std;


Mail::Mail()
{
    service_register_init_func(boost::bind(&Mail::mail_init, this, _1));
}

bool Mail::mail_init(const std::string& parm)
{
    register_callback();

    return true;
}

void Mail::service_start()
{
}

void Mail::register_callback()
{
    rpc_regester(pb::iTestPingMailREQ::descriptor()->full_name(), boost::bind(&Mail::rpcs_iping_mail_req, this, _1));
}

void Mail::text_message(const void * msg, size_t sz, uint32_t source, int session)
{

}

void Mail::service_send_cluster(const Message& msg, const string& remote_node, const string& remote_service)
{
    char* data;
    uint32_t size;
    int32_t roomid = get_service_roomid();
    uint32_t source = 0;
    int64_t uid = m_process_uid;
    uint32_t type = SUBTYPE_PROTOBUF;
    int32_t session = 0;
    serialize_imsg_proxy(msg, data, size, uid, type, roomid, session, remote_node, remote_service, m_node_name, m_master_name);
    skynet_send_noleak(m_ctx, source, m_cservice_proxy, PTYPE_TEXT | PTYPE_TAG_DONTCOPY, 0, data, size);
}

Message* Mail::rpcs_iping_mail_req(Message* data)
{
    pb::iTestPingMailREQ * msg = dynamic_cast<pb::iTestPingMailREQ*>(data);
    LOG(INFO) << "rpcs_iping_mail_req msg:" << msg->ShortDebugString();

    pb::iTestPingMailRSP* rsp = new pb::iTestPingMailRSP();
    rsp->set_pong_msg("Hello shop, I am mail. this is rpc rsp");
    return rsp;
}