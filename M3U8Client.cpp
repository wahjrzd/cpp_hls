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
		if (!SleepConditionVariableCS(&m_tsCondition, &m_tsLock, 8 * 1000))
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
	return m_updateTime.load();
}

void M3U8Client::UpdateUpdateTime(long long t)
{
	m_updateTime.store(t);
}

int M3U8Client::HasNewM3U8Coming(const std::string& info)
{
	EnterCriticalSection(&m_tsLock);
	m_m3u8Info.push(info);
	LeaveCriticalSection(&m_tsLock);
	WakeConditionVariable(&m_tsCondition);
	return 0;
}

//int M3U8Client::HasNewTsComing(TsFileInfo& f)
//{
//	char xx[64];
//	sprintf_s(xx, "#EXTINF:%0.3f,\n%S?id=%s\n", f.fileDuration / 1000.0f, f.fileName.c_str(), m_sessionID.c_str());
//
//	m_a[is] = xx;
//	++is;
//	if (is == 3)
//	{
//		char buf[256];
//		sprintf_s(buf, "%s#EXT-X-MEDIA-SEQUENCE:%u\n\n%s%s%s",
//			"#EXTM3U\n#EXT-X-VERSION:3\n#EXT-X-ALLOW-CACHE:NO\n#EXT-X-TARGETDURATION:2\n",
//			f.index - 2,
//			m_a[0].c_str(),
//			m_a[1].c_str(),
//			m_a[2].c_str());
//
//		HasNewM3U8Coming(buf);
//
//		is = 0;
//	}
//
//	
//	return 0;
//}
