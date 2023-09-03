#include <glog/logging.h>
#include "Shop.h"

extern "C"
{
#include "skynet.h"
#include "skynet_socket.h"
#include "skynet_server.h"
#include "skynet_handle.h"
#include "skynet_env.h"
}

extern "C"
{
    static int shop_cb(struct skynet_context * ctx, void * ud, int type, int session, uint32_t source, const void * msg, size_t sz)
    {
        Shop* shop = (Shop*)ud;
        shop->service_poll((const char*)msg, sz, source, session, type);
        return 0;
    }

    Shop* shop_create()
    {
        Shop* shop = new Shop();
        return shop;
    }

    void shop_release(Shop* shop)
    {
        delete shop;
    }

    int shop_init(Shop* shop, struct skynet_context* ctx, char* parm)
    {
        skynet_callback(ctx, shop, shop_cb);
        return 0;
    }
}