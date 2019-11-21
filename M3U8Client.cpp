#include "M3U8Client.h"
#include <chrono>

M3U8Client::M3U8Client(const std::string& id) :m_sessionID(id)
{
	auto tp = std::chrono::system_clock::now();
	auto t = std::chrono::system_clock::to_time_t(tp);
	
	UpdateUpdateTime(t);

	InitializeCriticalSection(&m_tsLock);
}

M3U8Client::~M3U8Client()
{
	DeleteCriticalSection(&m_tsLock);
}

std::string M3U8Client::GetM3U8()
{
	std::string s;
	EnterCriticalSection(&m_tsLock);
	if (m_m3u8Info.empty())
	{
		if (!SleepConditionVariableCS(&m_tsCondition, &m_tsLock, 7000))
			fprintf(stderr, "SleepConditionVariableCS failed:%u\n", GetLastError());
	}
	
	if (!m_m3u8Info.empty())
	{
		s = m_m3u8Info.front();
		m_m3u8Info.pop();
	}

	LeaveCriticalSection(&m_tsLock);

	auto tp = std::chrono::system_clock::now();
	auto t = std::chrono::system_clock::to_time_t(tp);

	UpdateUpdateTime(t);
	return s;
}

long long M3U8Client::GetUpdateTime()
{
	return m_updateTime;
}

void M3U8Client::UpdateUpdateTime(long long t)
{
	InterlockedExchange64(&m_updateTime, t);
}

int M3U8Client::HasNewM3U8Coming(const std::string& info)
{
	EnterCriticalSection(&m_tsLock);
	m_m3u8Info.push(info);
	LeaveCriticalSection(&m_tsLock);
	WakeConditionVariable(&m_tsCondition);
	return 0;
}
