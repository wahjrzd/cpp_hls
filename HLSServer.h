#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <http.h>
//#include <string>
#include <map>
#include <cpprest/http_listener.h>

const std::string textHtml = "text/html";
const std::string appMpegUrl = "application/x-mpegURL";
const utility::string_t videoMP2T = U("video/MP2T");
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
	void HandeHttpGet(web::http::http_request msg);

	void ParseUrl(const std::string& url, std::string& streamID, std::string& fn, std::string& uuid);
private:
	static unsigned __stdcall StaticCheck(void* arg);
	unsigned WrapCheck();

private:
	HANDLE m_checkThr;
	HANDLE m_checkEvent;
private:
	web::http::experimental::listener::http_listener* m_pListener;

	unsigned short m_port;
	std::wstring m_ip;

	utility::string_t m_uri;
private:
	CRITICAL_SECTION m_disLock;
	std::map<std::string, StreamDistribution*> m_distributions;
};

