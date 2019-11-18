#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <http.h>
#include <string>
#include <map>

const std::string textHtml = "text/html";
const std::string appMpegUrl = "application/x-mpegURL";
const std::string videoMP2T = "video/MP2T";
const std::string OK = "OK";
const std::string NotFound = "Not Found";

struct TaskParam
{
	void* server;
	PHTTP_REQUEST req;
	int reserve;
};

class StreamDistribution;

class HLSServer
{
public:
	HLSServer(const std::wstring& ip, unsigned short port);
	~HLSServer();

	void Start();
	void Stop();
private:
	DWORD SendHttpResponse(PHTTP_REQUEST req, USHORT StatusCode, std::string& pReason, std::string& EntityString);

	DWORD SendM3U8Response(PHTTP_REQUEST req, const std::string& m3u8);
	DWORD SendTSResponse(PHTTP_REQUEST req, const std::string& fn, const std::string& sessionID);

	void ParseUrl(const std::string& url, std::string& streamID, std::string& fn, std::string& uuid);
private:
	static void __cdecl StaticTask(void* arg);
	void RequestTask(PHTTP_REQUEST request);

	static unsigned __stdcall StaticCheck(void* arg);
	unsigned WrapCheck();
private:
	HANDLE m_checkThr;
	HANDLE m_checkEvent;
private:
	HANDLE m_reqQueue;
	unsigned short m_port;
	std::wstring m_ip;

	std::wstring m_uri;
	std::wstring m_queueName;
private:
	CRITICAL_SECTION m_disLock;
	std::map<std::string, StreamDistribution*> m_distributions;
};

