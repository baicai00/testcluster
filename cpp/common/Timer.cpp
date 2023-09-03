#include "common.h"
#include "Timer.h"
#include "Service.h"

extern "C"
{
#include "skynet_socket.h"
#include "skynet_server.h"
#include "skynet.h"
#include "skynet_timer.h"
#include "skynet_handle.h"
#include "skynet_env.h"
}
#define MAX_TIMER_IDX 0x7fffffff


Timer::Timer()
{
	m_t_idx = 1;
	m_service = NULL;
}

void Timer::timer_init(Service* service)
{
	m_service = service;
}

int Timer::start_timer(int time, const TimerCallBack& func)
{
	/*int id = m_t_idx++; // session
	if (m_t_idx >= MAX_TIMER_IDX || m_t_idx < 0)
		m_t_idx = 1;*/
	// skynet_timeout(uint32_t handle, int time, int session)
	// message.sz = (size_t)PTYPE_RESPONSE << MESSAGE_TYPE_SHIFT;
	int session = skynet_context_newsession(m_service->m_ctx);
	//LOG(INFO) << "start_timer skynet_context_newsession session = " << session;

	skynet_timeout(skynet_context_handle(m_service->m_ctx), time, session);
	auto& info = m_t[session];
	info.func = func;
	info.uid = m_service->m_process_uid;
	return session;
}

void Timer::stop_timer(int& id)
{
	m_t.erase(id);
	id = 0;
}

void Timer::timer_timeout(int id)
{
	
	auto it = m_t.find(id);
	if (it == m_t.end())
		return;
	
	m_service->m_process_uid = it->second.uid;
	it->second.func();
	m_service->m_process_uid = 0;
	m_t.erase(id);
}
