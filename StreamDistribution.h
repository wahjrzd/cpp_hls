#pragma once
#include <map>
#include <queue>
#include "RtspClient/RtpUnpacket.h"
#include "TsPacker.h"
#include "httpflv/FLVPacker.h"
#include <future>

class RtspClient;
class M3U8Client;
class FLVClient;

class StreamDistribution
{
public:
	StreamDistribution(const std::string& id, std::string& url);
	~StreamDistribution();

	int Run();

	void AddClient(const std::string& id, M3U8Client* cli);
	M3U8Client* GetClient(const std::string& id);
	size_t GetClientCount();

	void AddFLVClient(const std::string& id, FLVClient* cli);
	FLVClient* GetFLVClient(const std::string& id);
	void RemoveFLVClient(const std::string& id);

	void flvInputData(const FrameInfo& f)
	{
		if (m_startFLVPacket.load())
		{
			if (f.mediaType == "video")
				flvPacker->deliverVideoESPacket(f.data, f.timeStamp, f.frameType == 5 ? true : false);
			else
				flvPacker->deliverAudioESPacket(f.data, f.timeStamp / (f.samplingRate / 1000));
		}
	}
private:
	static unsigned rawcb(FrameInfo& f, void* arg);
	unsigned wrapRawCB(FrameInfo& f);

	unsigned WrapPacket();

	static unsigned tscb(TsFileInfo& f, void *arg);
	unsigned wrapTSCB(TsFileInfo& f);

	static unsigned flvcb(FLVFramePacket& f, void* arg);
	unsigned wrapFlvcb(FLVFramePacket& f);

	unsigned WrapCheck();
private:
	RtspClient* pstream;
	std::string m_streamID;
	std::string m_uri;
private:
	TsPacker* pPacker;
	std::wstring m_dir;
	std::atomic_bool m_startTSPacket;
private:
	FLVPacker *flvPacker;
	std::atomic_bool m_startFLVPacket;
private:
	std::future<unsigned int> m_checkFuture;
	HANDLE m_checkEvent;
private:
	CRITICAL_SECTION m_clientLock;
	std::map<std::string, M3U8Client*> m_clients;

	CRITICAL_SECTION m_flvClientLock;
	std::map<std::string, FLVClient*> m_flvClients;

	std::future<unsigned int> m_packetFuture;
	CRITICAL_SECTION m_frameLock;
	CONDITION_VARIABLE m_frameCondition;
	std::queue<FrameInfo> m_frames;

	std::queue<std::wstring> m_tsFiles;

	char items[3][64];
	unsigned int m_index;
	std::string m_header;

	FILE* pf;
};

