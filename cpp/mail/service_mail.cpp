#include <glog/logging.h>
#include "Mail.h"

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
    static int mail_cb(struct skynet_context * ctx, void * ud, int type, int session, uint32_t source, const void * msg, size_t sz)
    {
        Mail* mail = (Mail*)ud;
        mail->service_poll((const char*)msg, sz, source, session, type);
        return 0;
    }

    Mail* mail_create()
    {
        Mail* mail = new Mail();
        return mail;
    }

    void mail_release(Mail* mail)
    {
        delete mail;
    }

    int mail_init(Mail* mail, struct skynet_context* ctx, char* parm)
    {
        if (!mail->service_init(ctx, parm, strlen_1(parm))) {
            return -1;
        }
        skynet_callback(ctx, mail, mail_cb);
        return 0;
    }
}