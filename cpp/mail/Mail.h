#ifndef __MAIL_H__
#define __MAIL_H__

#include <stdint.h>
#include <string>
#include "Service.h"

class Mail : public Service
{
public:
    Mail();

    bool mail_init(const std::string& parm);

    virtual void service_start();

    void register_callback();

    void text_message(const void * msg, size_t sz, uint32_t source, int session);

    void service_send_cluster(const Message& msg, const string& remote_node, const string& remote_service);

    Message* rpcs_iping_mail_req(Message* data);

private:
    const std::string m_node_name = "mail_master_6";
    const std::string m_master_name = "mail_master";
};

#endif