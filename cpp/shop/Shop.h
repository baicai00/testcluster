#ifndef __SHOP_H__
#define __SHOP_H__

#include <stdint.h>
#include <string>
#include "Service.h"

class Shop : public Service
{
public:
    Shop();

    bool shop_init(const std::string& parm);

    virtual void service_start();

    void register_callback();

    void text_message(const void * msg, size_t sz, uint32_t source, int session);

    void proto_test_ping_shop(Message* data, uint32_t source);

    void service_send_cluster(const Message& msg, const string& remote_node, const string& remote_service);

private:
    const std::string m_node_name = "shop_master_4";
    const std::string m_master_name = "shop_master";
};

#endif