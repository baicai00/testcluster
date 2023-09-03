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
};

#endif