#include "skynet_wrapper.h"
#include <execinfo.h>

int skynet_send_noleak(struct skynet_context * context, uint32_t source, uint32_t destination , int type, int session, void * msg, size_t sz)
{
    if( destination == 0){
        //skynet_error(context, "Destination address can't be 0");
        if(msg && (type&PTYPE_TAG_DONTCOPY)){
            skynet_free(msg);
            return -1;
        }
        return session;
    }
    
    return skynet_send(context, source, destination, type, session, msg, sz);
}