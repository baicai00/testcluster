#include "Shop.h"
#include "common.h"
#include "pb/inner.pb.h"
#include <string>
using namespace std;


Shop::Shop()
{
    service_register_init_func(boost::bind(&Shop::shop_init, this, _1));
}

bool Shop::shop_init(const std::string& parm)
{
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

    pb::iTestPingActivityREQ req;
    req.set_ping_msg("Hello activity, I am shop");
    uint32_t shop_master = skynet_handle_findname(".shop_master")
    rpc_call(shop_master, req, [this] (Message* data) mutable {
        pb::iTestPingActivityRSP * rsp = dynamic_cast<pb::iTestPingActivityRSP*>(data);
        LOG(INFO) << "iTestPingActivityRSP:" << rsp->ShortDebugString();
    });
    // rpc_call需要修改，目前打算将消息转到shop_master后，再由shop_master转发到其他服务，因此rpc_call中需要带上目标地址
    // todo
}
