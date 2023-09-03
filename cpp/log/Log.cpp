#include "common.h"
#include "Log.h"
#include <fstream>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>

extern "C"
{
#include "skynet_socket.h"
#include "skynet_server.h"
#include "skynet.h"
#include "skynet_timer.h"
#include "skynet_handle.h"
#include "skynet_env.h"
}

using namespace std;

#define LOG_TIME_LENGTH 22


//static void timetostr(time_t time, char* str)
//{
//	struct tm* t = localtime(&time);
//	t->tm_year += 1900;
//	t->tm_mon += 1;
//	sprintf(str, "[%d-%02d-%02d %02d:%02d:%02d] ", t->tm_year, t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
//}





void log_debug(const char *msg, ...)
{
	char tmp[1024];
	//timetostr(time(NULL), tmp);
	va_list ap;
	va_start(ap, msg);
	vsnprintf(tmp, sizeof(tmp), msg, ap);
	va_end(ap);
	//skynet_error(NULL, tmp);
	LOG(INFO) << tmp;
}

void log_error(const char *msg, ...)
{
	char tmp[1024];
	//timetostr(time(NULL), tmp);
	va_list ap;
	va_start(ap, msg);
	vsnprintf(tmp, sizeof(tmp), msg, ap);
	va_end(ap);
	//skynet_error(NULL, tmp);
	LOG(ERROR) << tmp;
}

void log_info(const char *msg, ...)
{
	char tmp[1024];
	//timetostr(time(NULL), tmp);
	va_list ap;
	va_start(ap, msg);
	vsnprintf(tmp, sizeof(tmp), msg, ap);
	va_end(ap);
	//skynet_error(NULL, tmp);
	LOG(WARNING) << tmp;
}

void log_record(const std::string& msg)
{
	int fd = open("record.log", O_APPEND | O_WRONLY | O_CREAT, 0777);
	if (fd < 0)
	{
		log_error("open record.log error,error:%s", strerror(errno));
	}
	write(fd, msg.c_str(), msg.size());
	close(fd);
}


