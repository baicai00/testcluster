#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <glog/logging.h>
#include <string>

class logger
{
public:
};


extern "C"
{
#include "skynet.h"
#include "skynet_env.h"

struct logger* logger_create(void)
{
	struct logger * inst = (logger *)skynet_malloc(sizeof(*inst));
	return inst;
}

void logger_release(struct logger * inst)
{
	skynet_free(inst);
}

static int _logger(struct skynet_context * context, void *ud, int type, int session, uint32_t source, const void * msg, size_t sz)
{
	// char temp[100];
	// if (source != 0)
	// {
	// 	sprintf(temp, "[%08x] ", source);
	// 	LOG(INFO) << temp << (const char*)msg;
	// }
	// else
	// {
	// 	LOG(INFO) << (const char*)msg;
	// }

    char temp[100];
    char test[5] = {0};
    int testlen = sizeof(test);
    char* data = (char*)msg;
    snprintf(test, testlen, "%.*s", testlen-1, data);
    std::string prefix_name = test;
    //LOG(INFO) << " prefix_name:" << prefix_name;
    if (prefix_name == "BEEI")
    {
        strsep(&data, " "); // 去掉"BEEI"标识
        if (source != 0)
        {
            sprintf(temp, "[%08x] ", source);
            LOG(INFO) << temp << data;
        }
        else
        {
            LOG(INFO) << data;
        }
    }
    else if (prefix_name == "BEEW")
    {
        strsep(&data, " ");
        if (source != 0)
        {
            sprintf(temp, "[%08x] ", source);
            LOG(WARNING) << temp << data;
        }
        else
        {
            LOG(WARNING) << data;
        }
    }
    else if (prefix_name == "BEEE")
    {
        strsep(&data, " ");
        if (source != 0)
        {
            sprintf(temp, "[%08x] ", source);
            LOG(ERROR) << temp << data;
        }
        else
        {
            LOG(ERROR) << data;
        }
    }
    else
    {
        if (source != 0)
        {
            sprintf(temp, "[%08x] ", source);
            LOG(INFO) << temp << data;
        }
        else
        {
            LOG(INFO) << data;
        }
    }

	return 0;
}

int	logger_init(struct logger * inst, struct skynet_context *ctx, const char * parm)
{
	const char* dir = skynet_getenv("log_dir");
	const char* name = skynet_getenv("log_name");
	if (strcmp(name, "stdout") == 0)
	{
		FLAGS_logtostderr = true;
	}
	int logbufsecs = atoi(skynet_getenv("logbufsecs"));
	FLAGS_log_dir = dir;
	FLAGS_logbufsecs = logbufsecs;
	FLAGS_max_log_size = 10; // 单位M
	google::InitGoogleLogging(name);
	// log_producer_post_logs();
	skynet_callback(ctx, inst, _logger);
	skynet_command(ctx, "REG", ".logger");
	return 0;
}

}
