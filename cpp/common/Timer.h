#ifndef __TIMER_H__
#define __TIMER_H__
#include <map>
#include <stdint.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>
typedef boost::function<void()> TimerCallBack;
class Service;
struct TimerInfo
{
	TimerCallBack func;
	int64_t uid;
};
class skynet_context;
class Timer
{
public:
	Timer();
	void timer_init(Service* service);
	int start_timer(int time, const TimerCallBack& func);
	void stop_timer(int& id);
	void timer_timeout(int id);
private:
	int m_t_idx;
	Service* m_service;
	std::map<int, TimerInfo> m_t;
};

#endif