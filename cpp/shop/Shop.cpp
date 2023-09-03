#include "Shop.h"
#include "common.h"
#include <string>
using namespace std;


Shop::Shop()
{

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
