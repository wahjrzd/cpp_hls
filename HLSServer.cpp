#include "pch.h"
#include "HLSServer.h"
#include "StreamDistribution.h"
#include "WinUtility.h"
#include "M3U8Client.h"
#include <cpprest/filestream.h>
#include "Config.h"

#pragma warning(disable:4996)

HLSServer::HLSServer() :m_pListener(nullptr)
{
	InitializeCriticalSection(&m_disLock);

	m_checkEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_pCfg = new Config();
}

HLSServer::~HLSServer()
{
	SetEvent(m_checkEvent);
	if (m_checkFuture.valid())
		m_checkFuture.get();

	auto it = m_distributions.begin();
	while (it != m_distributions.end())
	{
		delete it->second;
		it = m_distributions.erase(it);
	}

	CloseHandle(m_checkEvent);
	DeleteCriticalSection(&m_disLock);

	delete m_pCfg;
}

void HLSServer::Start()
{
	if (!m_pCfg->LoadConfig(U("config.json")))
		return;

	m_ip = m_pCfg->_ip;
	m_port = m_pCfg->_port;

	WCHAR uri[MAX_PATH];
	swprintf_s(uri, MAX_PATH, L"http://%ws:%hu/", m_ip.c_str(), m_port);
	m_uri = uri;
	m_pListener = new web::http::experimental::listener::http_listener(m_uri);

	try
	{
		m_pListener->support(web::http::methods::GET, std::bind(&HLSServer::HandeHttpGet, this, std::placeholders::_1));
		m_pListener->open().wait();
		ucout << L"start lisen on " << m_pListener->uri().to_string() << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return;
	}

	if (!m_checkFuture.valid())
	{
		m_checkFuture = std::async(&HLSServer::WrapCheck, this);
	}
}

void HLSServer::HandeHttpGet(web::http::http_request msg)
{
	std::string streamid;
	std::string fn;
	std::string uuid;

	auto uri = utility::conversions::to_utf8string(msg.relative_uri().to_string());
	ParseUrl(uri, streamid, fn, uuid);

#ifdef _DEBUG
	ucout << msg.relative_uri().to_string() << std::endl;
#endif // DEBUG
	
	if (!streamid.empty())
	{
		auto pos = fn.find('.');
		if (pos != fn.npos)
		{
			if (fn.substr(pos) == ".m3u8")
			{
				if (uuid.empty())
				{
					std::string url = m_pCfg->streamMap[streamid];
					if (url.empty())
					{
						std::vector<std::pair<utility::string_t, web::json::value>> kv;
						kv.push_back({ U("error"),web::json::value::string(U("specify stream not found")) });
						web::json::value v = web::json::value::object(kv);
						
						msg.reply(web::http::status_codes::NotFound, v);
						return;
					}

					uuid = WinUtility::CreateXID();
					EnterCriticalSection(&m_disLock);
					auto it = m_distributions.find(streamid);
					if (it == m_distributions.end())
					{
						StreamDistribution* psd = new StreamDistribution(streamid, url);
						psd->Run();
						m_distributions.insert({ streamid,psd });

						M3U8Client* pm3u8 = new M3U8Client(uuid);
						psd->AddClient(uuid, pm3u8);
					}
					else
					{
						M3U8Client* pm3u8 = new M3U8Client(uuid);
						it->second->AddClient(uuid, pm3u8);
					}
					LeaveCriticalSection(&m_disLock);

					char m3u8[512];
					auto length = sprintf_s(m3u8, "#EXTM3U\n"
						"#EXT-X-STREAM-INF:PROGRAM-ID=1,BANDWIDTH=%d\n"
						"http://%ws:%hu/live/%s/%s?id=%s\n", 2000 * 1024, m_ip.c_str(), m_port, streamid.c_str(),
						fn.c_str(), uuid.c_str());
					
					msg.reply(web::http::status_codes::OK, m3u8, appMpegUrl);
				}
				else
				{
					M3U8Client* pm3u8 = nullptr;
					EnterCriticalSection(&m_disLock);
					auto it = m_distributions.find(streamid);
					if (it != m_distributions.end())
						pm3u8 = it->second->GetClient(uuid);
					LeaveCriticalSection(&m_disLock);

					std::string info = pm3u8->GetM3U8();
					
					if (info.empty())
					{
						std::vector<std::pair<utility::string_t, web::json::value>> kv;
						kv.push_back({ U("info"),web::json::value::string(U("m3u8 not already ok")) });
						web::json::value v = web::json::value::object(kv);

						msg.reply(web::http::status_codes::RequestTimeout, v);
					}
					else
						msg.reply(web::http::status_codes::OK, info, appMpegUrl);
				}
			}
			else if (fn.substr(pos) == ".ts")
			{
				std::string path = streamid + "/" + fn;
				
				auto fullPath = utility::conversions::usascii_to_utf16(path);
				try
				{
					concurrency::streams::fstream::open_istream(fullPath, std::ios::in|std::ios::binary).then(
						[=](concurrency::streams::istream is) {

						msg.reply(web::http::status_codes::OK, is, videoMP2T);

					}
					).get();
				}
				catch (const std::exception& e)
				{
					std::vector<std::pair<utility::string_t, web::json::value>> kv;
					kv.push_back({ U("error"),web::json::value::string(U("request this file some error happened")) });
					web::json::value v = web::json::value::object(kv);
					msg.headers().add(U("Connection"), U("close"));
					msg.reply(web::http::status_codes::NotFound, v);
					std::wcout << e.what() << std::endl;
				}
			}
			else
			{
				
			}
		}
	}
	else
	{
		msg.reply(web::http::status_codes::OK, "<h1>hello world</h1>", "text/html");
	}
}

void HLSServer::Stop()
{
	if (m_pListener)
	{
		try
		{
			m_pListener->close().wait();
		}
		catch (const std::exception&)
		{

		}
		delete m_pListener;
	}
}

void HLSServer::ParseUrl(const std::string& url, std::string& streamID, std::string& fn, std::string& uuid)
{

	char* p1 = new char[url.size()];
	char* p2 = new char[url.size()];
	char* p3 = new char[url.size()];

	if (sscanf(url.c_str(), "/live/%[^/]/%[^?]?id=%s", p1, p2, p3) == 3)
	{
		streamID = p1;
		fn = p2;
		uuid = p3;
	}
	else  if (sscanf(url.c_str(), "/live/%[^/]/%s", p1, p2) == 2)
	{
		streamID = p1;
		fn = p2;
	}
	else
	{
		/*TODO*/
	}
	delete p1;
	delete p2;
	delete p3;
}

unsigned int HLSServer::WrapCheck()
{
	DWORD dwRet;
	while (true)
	{
		dwRet = WaitForSingleObject(m_checkEvent, 10 * 1000);
		if (dwRet == WAIT_TIMEOUT)
		{
			EnterCriticalSection(&m_disLock);
			auto it = m_distributions.begin();
			while (it != m_distributions.end())
			{
				if (it->second->GetClientCount() == 0)
				{
					delete it->second;
					it = m_distributions.erase(it);
					std::clog << "remove distribution" << std::endl;
					continue;
				}
				++it;
			}
			LeaveCriticalSection(&m_disLock);
		}
		else
			break;
	}
	return 0;
}