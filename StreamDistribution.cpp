#include "StreamDistribution.h"
#include "RtspClient/RtspClient.h"
#include <iostream>
#include "WinUtility.h"
#include "M3U8Client.h"
#include "httpflv/FLVClient.h"
#include <time.h>

StreamDistribution::StreamDistribution(const std::string& id, std::string& url) :pstream(nullptr),
m_uri(url),
m_streamID(id),
m_index(0),
m_startTSPacket(false),
m_startFLVPacket(false)
{
	InitializeCriticalSection(&m_clientLock);
	InitializeCriticalSection(&m_flvClientLock);
	InitializeCriticalSection(&m_frameLock);
	InitializeConditionVariable(&m_frameCondition);

	m_dir = WinUtility::AnisToUnicode(id.c_str(), id.size());
	m_dir.push_back('/');

	pPacker = new TsPacker(m_dir);
	pPacker->SetCallback(tscb, this);

	flvPacker = new FLVPacker();
	flvPacker->SetCallback(flvcb, this);

	m_header = "#EXTM3U\n#EXT-X-VERSION:3\n#EXT-X-ALLOW-CACHE:NO\n#EXT-X-TARGETDURATION:4\n";

	m_checkEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	//fopen_s(&pf, "1.flv", "wb");
}

StreamDistribution::~StreamDistribution()
{
	if (pstream)
	{
		pstream->Stop();
		delete pstream;
	}

	EnterCriticalSection(&m_frameLock);
	FrameInfo f;
	m_frames.push(f);
	LeaveCriticalSection(&m_frameLock);
	WakeConditionVariable(&m_frameCondition);

	if (m_packetFuture.valid())
		m_packetFuture.get();

	SetEvent(m_checkEvent);

	if (m_checkFuture.valid())
		m_checkFuture.get();

	while (!m_tsFiles.empty())
	{
		std::wstring fullpath = m_dir + m_tsFiles.front();
		DeleteFile(fullpath.c_str());
		m_tsFiles.pop();
	}

	delete pPacker;
	delete flvPacker;
	
	CloseHandle(m_checkEvent);
	DeleteCriticalSection(&m_clientLock);
	DeleteCriticalSection(&m_flvClientLock);
	DeleteCriticalSection(&m_frameLock);

	//fclose(pf);
}

void StreamDistribution::AddClient(const std::string& id, M3U8Client* cli)
{
	m_startTSPacket.store(true);

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

	if (!m_packetFuture.valid())
	{
		m_packetFuture = std::async(&StreamDistribution::WrapPacket, this);
	}
	
	if (!m_checkFuture.valid())
	{
		m_checkFuture = std::async(&StreamDistribution::WrapCheck, this);
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

		auto task = concurrency::create_task([&]() {
			this->flvInputData(f);
			return 0;
		});

		if (m_startTSPacket.load())
		{
			if (f.mediaType == "video")
			{
				if (f.frameType == 5) {
					pPacker->deliverVideoESPacket(f.data.c_str(), f.data.size(), f.timeStamp, true);
				}
				else if (f.frameType != 0)
					pPacker->deliverVideoESPacket(f.data.c_str(), f.data.size(), f.timeStamp, false);
				else
					break;
			}
		}
		

		task.get();
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

unsigned StreamDistribution::flvcb(FLVFramePacket& f, void* arg)
{
	StreamDistribution* p = (StreamDistribution*)arg;
	return p->wrapFlvcb(f);
}

unsigned StreamDistribution::wrapFlvcb(FLVFramePacket& f)
{
	EnterCriticalSection(&m_flvClientLock);
	auto it = m_flvClients.begin();
	if (it != m_flvClients.end())
	{
		if (it->second->newClient)
		{
			auto aa = f.MetaFunc(f.arg);
			auto bb = f.VideoSeqFunc(f.arg);
			auto dd = f.AudioSeqFunc(f.arg);
			std::basic_string<std::uint8_t> cc(FLV_FILE_HEADER, 13);// = FLV_FILE_HEADER;
			cc.append(aa);
			cc += bb;
			cc += dd;
			it->second->HasNewFLVTag(cc);

			//fwrite(cc.c_str(), 1, cc.size(), pf);
			it->second->newClient = false;
		}
		it->second->HasNewFLVTag(f.data);

		//fwrite(f.data.c_str(), 1, f.data.size(), pf);
	}
	LeaveCriticalSection(&m_flvClientLock);
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
	count += m_clients.size();
	LeaveCriticalSection(&m_clientLock);

	EnterCriticalSection(&m_flvClientLock);
	count += m_flvClients.size();
	LeaveCriticalSection(&m_flvClientLock);
	return count;
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
					std::clog << "remove client:" << it->second->GetSessionID() << std::endl;
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

void StreamDistribution::AddFLVClient(const std::string& id, FLVClient* cli)
{
	m_startFLVPacket.store(true);

	EnterCriticalSection(&m_flvClientLock);
	auto it = m_flvClients.find(id);
	if (it != m_flvClients.end())
	{
		LeaveCriticalSection(&m_flvClientLock);
		std::cout << "already exists" << std::endl;
		return;
	}
	m_flvClients.insert({ id,cli });
	LeaveCriticalSection(&m_flvClientLock);
}

FLVClient* StreamDistribution::GetFLVClient(const std::string& id)
{
	FLVClient* flv = nullptr;
	EnterCriticalSection(&m_flvClientLock);
	auto it = m_flvClients.find(id);
	if (it != m_flvClients.end())
		flv = it->second;
	LeaveCriticalSection(&m_flvClientLock);
	return flv;
}

void StreamDistribution::RemoveFLVClient(const std::string& id)
{
	EnterCriticalSection(&m_flvClientLock);
	m_flvClients.erase(id);
	if (m_flvClients.empty())
		m_startFLVPacket.store(false);
	LeaveCriticalSection(&m_flvClientLock);
}