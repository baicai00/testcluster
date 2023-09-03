#ifndef __SKYNET_WRAPPER_H__
#define __SKYNET_WRAPPER_H__

#ifdef __cplusplus
extern "C"
{
#endif
#include "skynet_server.h"
#include "skynet.h"

// do not use skynet_send directly!!!
// before v.1.2.0, mem leak when call skynet_send with destination(0)
int skynet_send_noleak(struct skynet_context * context, uint32_t source, uint32_t destination , int type, int session, void * msg, size_t sz);


#ifdef __cplusplus
}
#endif

#endif