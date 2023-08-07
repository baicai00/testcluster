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
	char temp[100];
    char test[5] = {0};
    int testlen = sizeof(test);
    char* data = (char*)msg;
    snprintf(test, testlen, "%.*s", testlen-1, data);
    std::string prefix_name = test;
    //LOG(INFO) << " prefix_name:" << prefix_name;
	if (source != 0)
	{
		sprintf(temp, "[%08x] ", source);
		LOG(INFO) << temp << (const char*)msg;
	}
	else
	{
		LOG(INFO) << (const char*)msg;
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
	skynet_callback(ctx, inst, _logger);
	skynet_command(ctx, "REG", ".logger");
	return 0;
}

}
