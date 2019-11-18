#pragma once
#include <queue>
#include "TsPacker.h"

class M3U8Client
{
public:
	explicit M3U8Client(const std::string& id);
	~M3U8Client();

	int HasNewM3U8Coming(const std::string& info);

	std::string GetM3U8();

	long long GetUpdateTime();
	void UpdateUpdateTime(long long t);
private:
	std::string m_sessionID;
	volatile long long  m_updateTime;
	CONDITION_VARIABLE m_tsCondition;
	CRITICAL_SECTION m_tsLock;
	std::queue<std::string> m_m3u8Info;
};

