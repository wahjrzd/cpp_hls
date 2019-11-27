#pragma once
#include <queue>
#include "TsPacker.h"
#include <atomic>

class M3U8Client
{
public:
	explicit M3U8Client(const std::string& id);
	~M3U8Client();

	int HasNewM3U8Coming(const std::string& info);

	//int HasNewTsComing(TsFileInfo& info);

	std::string GetM3U8();
	 
	long long GetUpdateTime();

	const std::string GetSessionID()
	{
		return m_sessionID;
	}

	void UpdateUpdateTime(long long t);
private:
	std::string m_sessionID;
	std::atomic_int64_t m_updateTime;
	CONDITION_VARIABLE m_tsCondition;
	CRITICAL_SECTION m_tsLock;
	std::queue<std::string> m_m3u8Info;

	//std::string m_a[3];
	//int is = 0;
};

