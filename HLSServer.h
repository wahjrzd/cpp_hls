#pragma once
#include <WinSock2.h>
#include <map>
#include <cpprest/http_listener.h>
#include <future>

const std::string textHtml = "text/html";
const std::string appMpegUrl = "application/x-mpegURL";
const utility::string_t videoMP2T = U("video/MP2T");

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
	unsigned int WrapCheck();
private:
	std::future<unsigned int> m_checkFuture;
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

