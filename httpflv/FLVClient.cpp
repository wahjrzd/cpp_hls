#include "FLVClient.h"

FLVClient::FLVClient() :newClient(true)
{
	InitializeCriticalSection(&m_queueLock);
	InitializeConditionVariable(&m_queueCondition);
}

FLVClient::~FLVClient()
{
	DeleteCriticalSection(&m_queueLock);
}

void  FLVClient::HasNewFLVTag(const std::basic_string<std::uint8_t>& data)
{
	EnterCriticalSection(&m_queueLock);
	tagQueue.push(data);
	LeaveCriticalSection(&m_queueLock);

	WakeConditionVariable(&m_queueCondition);
}

std::basic_string<std::uint8_t> FLVClient::GetTagData()
{
	std::basic_string<std::uint8_t> s;
	EnterCriticalSection(&m_queueLock);
	if (tagQueue.empty())
		SleepConditionVariableCS(&m_queueCondition, &m_queueLock, 2000);

	if (!tagQueue.empty())
	{
		s = tagQueue.front();
		tagQueue.pop();
	}
	LeaveCriticalSection(&m_queueLock);
	return s;
}