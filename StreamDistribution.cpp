#include "StreamDistribution.h"
#include "RtspClient/RtspClient.h"
#include <iostream>
#include <process.h>
#include "WinUtility.h"
#include "M3U8Client.h"
#include <time.h>

StreamDistribution::StreamDistribution(const std::string& id, std::string& url) :m_streamSourceThr(NULL),
m_checkThr(NULL),
pstream(nullptr),
m_uri(url),
m_streamID(id),
m_index(0)
{
	InitializeCriticalSection(&m_clientLock);
	InitializeCriticalSection(&m_frameLock);
	InitializeConditionVariable(&m_frameCondition);
	
	m_dir = WinUtility::AnisToUnicode(id.c_str(), id.size());
	m_dir.push_back('/');

	pPacker = new TsPacker(m_dir);
	pPacker->SetCallback(tscb, this);

	m_header = "#EXTM3U\n#EXT-X-VERSION:3\n#EXT-X-ALLOW-CACHE:NO\n#EXT-X-TARGETDURATION:4\n";

	m_checkEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

StreamDistribution::~StreamDistribution()
{
	if (pstream)
	{
		pstream->Stop();
		delete pstream;
	}

	delete pPacker;

	FrameInfo f;
	f.frameType = 0;
	m_frames.push(f);
	WakeConditionVariable(&m_frameCondition);

	if (m_streamSourceThr)
	{
		WaitForSingleObject(m_streamSourceThr, INFINITE);
		CloseHandle(m_streamSourceThr);
	}

	SetEvent(m_checkEvent);

	if (m_checkThr)
	{
		WaitForSingleObject(m_checkThr, INFINITE);
		CloseHandle(m_checkThr);
	}

	while (!m_tsFiles.empty())
	{
		std::wstring fullpath = m_dir + m_tsFiles.front();
		DeleteFile(fullpath.c_str());
		m_tsFiles.pop();
	}

	CloseHandle(m_checkEvent);
	DeleteCriticalSection(&m_clientLock);
	DeleteCriticalSection(&m_frameLock);
}

void StreamDistribution::AddClient(const std::string& id, M3U8Client* cli)
{
	EnterCriticalSection(&m_clientLock);
	auto it = m_clients.find(id);
	if (it != m_clients.end())
	{
		LeaveCriticalSection(&m_clientLock);
		std::cout << "already exists" << std::endl;
		return;
	}
	m_clients.insert({ id,cli });
	LeaveCriticalSection(&m_clientLock);
}

int StreamDistribution::Run()
{
	if (!pstream)
	{
		pstream = new RtspClient();
		if (pstream->OpenRtsp(m_uri.c_str()))
			pstream->SetRawCallback(rawcb, this);
		else
		{
			delete pstream;
			pstream = nullptr;
			return 1;
		}
	}

	if (m_streamSourceThr == NULL)
	{
		m_streamSourceThr = (HANDLE)_beginthreadex(nullptr, 0, StaticPacket, this, 0, nullptr);
		if (m_streamSourceThr == NULL)
		{
			fprintf(stderr, "_beginthreadex failed:%u\n", GetLastError());
			return 2;
		}
	}

	if (m_checkThr == NULL)
	{
		m_checkThr = (HANDLE)_beginthreadex(nullptr, 0, StaticCheck, this, 0, nullptr);
		if (m_checkThr == NULL)
		{
			fprintf(stderr, "_beginthreadex failed:%u\n", GetLastError());
			return 3;
		}
	}
	return 0;
}

unsigned StreamDistribution::rawcb(FrameInfo& f, void* arg)
{
	StreamDistribution* p = (StreamDistribution*)arg;
	return p->wrapRawCB(f);
}

unsigned StreamDistribution::wrapRawCB(FrameInfo& f)
{
	EnterCriticalSection(&m_frameLock);
	m_frames.push(f);
	LeaveCriticalSection(&m_frameLock);

	WakeConditionVariable(&m_frameCondition);
	return 0;
}

unsigned __stdcall StreamDistribution::StaticPacket(void* arg)
{
	StreamDistribution* p = (StreamDistribution*)arg;
	return p->WrapPacket();
}

unsigned StreamDistribution::WrapPacket()
{
	while (true)
	{
		EnterCriticalSection(&m_frameLock);
		while (m_frames.empty())
		{
			SleepConditionVariableCS(&m_frameCondition, &m_frameLock, INFINITE);
		}
		FrameInfo f = m_frames.front();
		m_frames.pop();
		LeaveCriticalSection(&m_frameLock);

		if (f.frameType == 5) {
			pPacker->deliverESPacket(f.data.c_str(), f.data.size(), f.timeStamp, true);
		}
		else if (f.frameType != 0)
			pPacker->deliverESPacket(f.data.c_str(), f.data.size(), f.timeStamp, false);
		else
			break;
	}
	return 0;
}

unsigned StreamDistribution::tscb(TsFileInfo& f, void *arg)
{
	StreamDistribution* p = (StreamDistribution*)arg;
	return p->wrapTSCB(f);
}

unsigned StreamDistribution::wrapTSCB(TsFileInfo& f)
{
	m_tsFiles.push(f.fileName);
	if (m_tsFiles.size() > 12)
	{
		int i = 0;
		while (i < 6)
		{
			std::wstring fullpath = m_dir + m_tsFiles.front();
			DeleteFile(fullpath.c_str());
			m_tsFiles.pop();
			i++;
		}
	}

	sprintf_s(items[m_index], "#EXTINF:%0.3f,\n%S\n", f.fileDuration / 1000.0f, f.fileName.c_str());
	m_index++;
	if (m_index == 3)
	{
		char buf[128];
		sprintf_s(buf, "%s#EXT-X-MEDIA-SEQUENCE:%u\n\n", m_header.c_str(), f.index - 2);

		std::string info = buf;
		int i = 0;
		while (i < 3)
		{
			info.append(items[i]);
			i++;
		}

		EnterCriticalSection(&m_clientLock);
		auto it = m_clients.begin();
		while (it != m_clients.end())
		{
			it->second->HasNewM3U8Coming(info);
			++it;
		}
		LeaveCriticalSection(&m_clientLock);

		m_index = 0;
	}

	return 0;
}

M3U8Client* StreamDistribution::GetClient(const std::string& id)
{
	M3U8Client* p = nullptr;
	EnterCriticalSection(&m_clientLock);
	auto it = m_clients.find(id);
	if (it != m_clients.end())
		p = it->second;
	LeaveCriticalSection(&m_clientLock);
	return p;
}

size_t StreamDistribution::GetClientCount()
{
	size_t count = 0;
	EnterCriticalSection(&m_clientLock);
	count = m_clients.size();
	LeaveCriticalSection(&m_clientLock);
	return count;
}

unsigned __stdcall StreamDistribution::StaticCheck(void* arg)
{
	StreamDistribution* p = (StreamDistribution*)arg;
	return p->WrapCheck();
}

unsigned StreamDistribution::WrapCheck()
{
	DWORD dwRet;
	while (true)
	{
		dwRet = WaitForSingleObject(m_checkEvent, 15000);
		if (WAIT_TIMEOUT == dwRet)
		{
			auto ct = time(NULL);
			EnterCriticalSection(&m_clientLock);
			auto it = m_clients.begin();
			while (it != m_clients.end())
			{
				if (difftime(ct, it->second->GetUpdateTime()) > 15)
				{
					printf("remove client\n");
					delete it->second;
					it = m_clients.erase(it);
					continue;
				}
				++it;
			}
			LeaveCriticalSection(&m_clientLock);
		}
		else
			break;
	}
	return 0;
}