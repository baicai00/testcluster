#ifndef __LOG__H__
#define __LOG__H__
#include <string>
#include <glog/logging.h>
//#include <glog/raw_logging.h>
//#include <g3sinks/LogRotate.h>
//#include <g3log/g3log.hpp>

extern "C"
{
#include "skynet_socket.h"
#include "skynet_server.h"
#include "skynet.h"
#include "skynet_timer.h"
#include "skynet_handle.h"
#include "skynet_env.h"
}


void log_error(const char *msg, ...);
void log_debug(const char *msg, ...);
void log_info(const char *msg, ...);

void log_record(const std::string& msg);



/*

#define PRINT_GOOGLE_LOG(level,format, args...) do{ \
    RAW_LOG( level, " [%x] func:%s() msg:" format , skynet_current_handle() ,__func__ ,  ##args); \
}while(0);

#define PGLOG_ERROR(format, args...) do{\
    PRINT_GOOGLE_LOG(ERROR, format, ##args); \
}while(0);

#define PGLOG_WARNING(format, args...) do{\
    PRINT_GOOGLE_LOG(WARNING, format, ##args); \
}while(0);

#define PGLOG_INFO(format, args...) do{\
    PRINT_GOOGLE_LOG(INFO, format, ##args); \
}while(0);

#define PGLOG_DEBUG(format, args...) do{\
	PGLOG_INFO(format, ##args); \
}while(0);

*/




#endif
