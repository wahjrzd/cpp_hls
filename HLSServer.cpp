#include "pch.h"
#include "HLSServer.h"
#include <process.h>
#include "StreamDistribution.h"
#include "WinUtility.h"
#include "M3U8Client.h"
#include <cpprest/filestream.h>

#pragma warning(disable:4996)

HLSServer::HLSServer(const std::wstring& ip, unsigned short port) : m_port(port), m_ip(ip), m_checkThr(NULL)
{
	WCHAR uri[MAX_PATH];
	swprintf_s(uri, MAX_PATH, L"http://%ws:%hu/", m_ip.c_str(), m_port);
	m_uri = uri;
	m_pListener = new web::http::experimental::listener::http_listener(m_uri);

	InitializeCriticalSection(&m_disLock);
	
	m_checkEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}


HLSServer::~HLSServer()
{
	SetEvent(m_checkEvent);
	if (m_checkThr)
	{
		WaitForSingleObject(m_checkThr, INFINITE);
		CloseHandle(m_checkThr);
	}
	CloseHandle(m_checkEvent);
	DeleteCriticalSection(&m_disLock);
}

void HLSServer::Start()
{
	try
	{
		m_pListener->support(web::http::methods::GET, std::bind(&HLSServer::HandeHttpGet, this, std::placeholders::_1));
		m_pListener->open().wait();
		ucout << L"start lisen on " << m_pListener->uri().to_string() << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	if (!m_checkThr)
	{
		m_checkThr = (HANDLE)_beginthreadex(nullptr, 0, StaticCheck, this, 0, nullptr);
		if (m_checkThr == NULL)
		{
			fprintf(stderr, "_beginthreadex failed:%u\n", GetLastError());
			return;
		}
	}
}

void HLSServer::HandeHttpGet(web::http::http_request msg)
{
	std::string streamid;
	std::string fn;
	std::string uuid;

	auto uri = utility::conversions::to_utf8string(msg.relative_uri().to_string());
	ParseUrl(uri, streamid, fn, uuid);

	if (!streamid.empty())
	{
		auto pos = fn.find('.');
		if (pos != fn.npos)
		{
			if (fn.substr(pos) == ".m3u8")
			{
				if (uuid.empty())
				{
					std::string url = "rtsp://admin:admin@25.30.9.45:554/cam/realmonitor?channel=1&subtype=0";

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

					int i = 0;
					std::string info = pm3u8->GetM3U8();
					
					if (info.empty())
						msg.reply(web::http::status_codes::RequestTimeout);
					else
						msg.reply(web::http::status_codes::OK, info, appMpegUrl);
					//web::json::value v;
				
				}
			}
			else if (fn.substr(pos) == ".ts")
			{
				std::string path = streamid + "/" + fn;
				
				auto fullPath = utility::conversions::usascii_to_utf16(path);
				try
				{
					concurrency::streams::fstream::open_istream(fullPath, std::ios::in).then(
						[=](concurrency::streams::istream is) {
						if (is.is_open())
							msg.reply(web::http::status_codes::OK, is, videoMP2T);
						else
						{
							msg.reply(web::http::status_codes::OK, U("MMP"));
							ucout << "TODO" << std::endl;
						}
					}
					).get();
				}
				catch (const std::exception& e)
				{
					//std::wcout << e.what() << std::endl;
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
	m_pListener->close().wait();
	delete m_pListener;
}

//void __cdecl HLSServer::StaticTask(void* arg)
//{
//	TaskParam* p = (TaskParam*)arg;
//	
//	((HLSServer*)p->server)->RequestTask(p->req);
//
//	delete p;
//}

//void HLSServer::RequestTask(PHTTP_REQUEST request)
//{
//	if (request->Verb == HttpVerbGET)
//	{
//		std::string streamid;
//		std::string fn;
//		std::string uuid;
//		ParseUrl(request->pRawUrl, streamid, fn, uuid);
//
//		if (!streamid.empty())
//		{
//			auto pos = fn.find('.');
//			if (pos != fn.npos)
//			{
//				if (fn.substr(pos) == ".m3u8")
//				{
//					if (uuid.empty())
//					{
//						std::string url = "rtsp://admin:admin@25.30.9.45:554/cam/realmonitor?channel=1&subtype=0";
//
//						uuid = WinUtility::CreateXID();
//						EnterCriticalSection(&m_disLock);
//						auto it = m_distributions.find(streamid);
//						if (it == m_distributions.end())
//						{
//							StreamDistribution* psd = new StreamDistribution(streamid, url);
//							psd->Run();
//							m_distributions.insert({ streamid,psd });
//
//							M3U8Client* pm3u8 = new M3U8Client(uuid);
//							psd->AddClient(uuid, pm3u8);
//						}
//						else
//						{
//							M3U8Client* pm3u8 = new M3U8Client(uuid);
//							it->second->AddClient(uuid, pm3u8);
//						}
//						LeaveCriticalSection(&m_disLock);
//
//						char m3u8[512];
//						auto length = sprintf_s(m3u8, "#EXTM3U\n"
//							"#EXT-X-STREAM-INF:PROGRAM-ID=1,BANDWIDTH=%d\n"
//							"http://%ws:%hu/live/%s/%s?id=%s\n", 2000 * 1024, m_ip.c_str(), m_port, streamid.c_str(),
//							fn.c_str(), uuid.c_str());
//
//						SendM3U8Response(request, m3u8);
//					}
//					else
//					{
//						M3U8Client* pm3u8 = nullptr;
//						EnterCriticalSection(&m_disLock);
//						auto it = m_distributions.find(streamid);
//						if (it != m_distributions.end())
//							pm3u8 = it->second->GetClient(uuid);
//						LeaveCriticalSection(&m_disLock);
//
//						int i = 0;
//						std::string info;
//						while (i < 120)
//						{
//							info = pm3u8->GetM3U8();
//							if (!info.empty())
//								break;
//							Sleep(40);
//							i++;
//						}
//						SendM3U8Response(request, info);
//					}
//				}
//				else if (fn.substr(pos) == ".ts")
//				{
//					SendTSResponse(request, fn, streamid);
//				}
//				else
//					printf("format TODO\n");
//			}
//		}
//		else
//		{
//			std::string reason = "OK";
//			std::string entity = "<h1>Hello WinHttp</h1>";
//			SendHttpResponse(request, 200, reason, entity);
//		}
//	}
//	else if (request->Verb == HttpVerbPOST)
//	{
//
//	}
//	else
//		printf("REQUEST TODO\n");
//
//	delete (PCHAR)request;
//}

//DWORD HLSServer::SendHttpResponse(PHTTP_REQUEST req, USHORT StatusCode, std::string& pReason, std::string& EntityString)
//{
//	HTTP_RESPONSE response;
//	HTTP_DATA_CHUNK dataChunk;
//	DWORD result;
//	DWORD bytesSent;
//
//	RtlZeroMemory(&response, sizeof(HTTP_RESPONSE));
//	response.StatusCode = StatusCode;
//	response.pReason = pReason.c_str();
//	response.ReasonLength = (USHORT)pReason.size();
//	response.Headers.KnownHeaders[HttpHeaderConnection].pRawValue = "close";
//	response.Headers.KnownHeaders[HttpHeaderConnection].RawValueLength = (USHORT)strlen("close");
//	response.Headers.KnownHeaders[HttpHeaderContentType].pRawValue = textHtml.c_str();
//	response.Headers.KnownHeaders[HttpHeaderContentType].RawValueLength = (USHORT)textHtml.size();
//
//	if (!EntityString.empty())
//	{
//		dataChunk.DataChunkType = HttpDataChunkFromMemory;
//		dataChunk.FromMemory.pBuffer = (PVOID)EntityString.c_str();
//		dataChunk.FromMemory.BufferLength = EntityString.size();
//		response.EntityChunkCount = 1;
//		response.pEntityChunks = &dataChunk;
//	}
//
//	result = HttpSendHttpResponse(m_reqQueue, req->RequestId, 0, &response, NULL, &bytesSent, NULL, 0, NULL, NULL);
//	if (result != NO_ERROR)
//		wprintf(L"HttpSendHttpResponse failed with %lu \n", result);
//
//	return result;
//}

//DWORD HLSServer::SendM3U8Response(PHTTP_REQUEST req, const std::string& m3u8)
//{
//	HTTP_RESPONSE response;
//	HTTP_DATA_CHUNK dataChunk;
//	DWORD result;
//	DWORD bytesSent;
//
//	RtlZeroMemory(&response, sizeof(HTTP_RESPONSE));
//	response.StatusCode = 200;
//	response.pReason = OK.c_str();
//	response.ReasonLength = (USHORT)OK.size();
//	response.Headers.KnownHeaders[HttpHeaderCacheControl].pRawValue = "no-cache";
//	response.Headers.KnownHeaders[HttpHeaderCacheControl].RawValueLength = (USHORT)strlen("no-cache");
//	response.Headers.KnownHeaders[HttpHeaderContentType].pRawValue = appMpegUrl.c_str();
//	response.Headers.KnownHeaders[HttpHeaderContentType].RawValueLength = (USHORT)appMpegUrl.size();
//	response.Headers.KnownHeaders[HttpHeaderConnection].pRawValue = "close";
//	response.Headers.KnownHeaders[HttpHeaderConnection].RawValueLength = (USHORT)strlen("close");
//
//	dataChunk.DataChunkType = HttpDataChunkFromMemory;
//	dataChunk.FromMemory.BufferLength = m3u8.size();
//	dataChunk.FromMemory.pBuffer = (PVOID)m3u8.c_str();
//
//	response.EntityChunkCount = 1;
//	response.pEntityChunks = &dataChunk;
//
//	result = HttpSendHttpResponse(m_reqQueue, req->RequestId, 0, &response, NULL, &bytesSent, NULL, 0, NULL, NULL);
//	if (result != NO_ERROR)
//		wprintf(L"HttpSendHttpResponse failed with %lu \n", result);
//
//	return result;
//}

//DWORD HLSServer::SendTSResponse(PHTTP_REQUEST req, const std::string& fn, const std::string& sessionID)
//{
//	HANDLE hFile;
//	HTTP_RESPONSE response;
//	HTTP_DATA_CHUNK dataChunk;
//	DWORD result;
//	DWORD bytesSent;
//
//	RtlZeroMemory(&response, sizeof(HTTP_RESPONSE));
//	std::string fullPath = sessionID + "/" + fn;
//	hFile = CreateFileA(fullPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
//	
//	if (hFile == INVALID_HANDLE_VALUE)
//	{
//		response.StatusCode = 404;
//		response.pReason = NotFound.c_str();
//		response.ReasonLength = (USHORT)NotFound.size();
//		result = HttpSendHttpResponse(m_reqQueue, req->RequestId, 0, &response, NULL, &bytesSent, NULL, 0, NULL, NULL);
//		if (result != NO_ERROR)
//			wprintf(L"HttpSendHttpResponse failed with %lu \n", result);
//		else
//			wprintf(L"file:%S not found\n", fn.c_str());
//		return 0;
//	}
//
//	char szFileLength[16];
//	LARGE_INTEGER fileSize;
//	GetFileSizeEx(hFile, &fileSize);
//	sprintf_s(szFileLength, 16, "%lld", fileSize.QuadPart);
//
//	response.StatusCode = 200;
//	response.pReason = OK.c_str();
//	response.ReasonLength = (USHORT)OK.size();
//	response.Headers.KnownHeaders[HttpHeaderCacheControl].pRawValue = "no-cache";
//	response.Headers.KnownHeaders[HttpHeaderCacheControl].RawValueLength = (USHORT)strlen("no-cache");
//	response.Headers.KnownHeaders[HttpHeaderContentType].pRawValue = videoMP2T.c_str();
//	response.Headers.KnownHeaders[HttpHeaderContentType].RawValueLength = (USHORT)videoMP2T.size();
//	response.Headers.KnownHeaders[HttpHeaderContentLength].pRawValue = szFileLength;
//	response.Headers.KnownHeaders[HttpHeaderContentLength].RawValueLength = (USHORT)strlen(szFileLength);
//
//	dataChunk.DataChunkType = HttpDataChunkFromFileHandle;
//	dataChunk.FromFileHandle.ByteRange.StartingOffset.QuadPart = 0;
//	dataChunk.FromFileHandle.ByteRange.Length.QuadPart = HTTP_BYTE_RANGE_TO_EOF;
//	dataChunk.FromFileHandle.FileHandle = hFile;
//
//	response.EntityChunkCount = 1;
//	response.pEntityChunks = &dataChunk;
//
//	result = HttpSendHttpResponse(m_reqQueue, req->RequestId, 0, &response, NULL, &bytesSent, NULL, 0, NULL, NULL);
//	if (result != NO_ERROR)
//		wprintf(L"HttpSendHttpResponse failed with %lu \n", result);
//
//	if (hFile)
//		CloseHandle(hFile);
//
//	return result;
//}

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

unsigned __stdcall HLSServer::StaticCheck(void* arg)
{
	HLSServer* p = (HLSServer*)arg;
	return p->WrapCheck();
}
unsigned HLSServer::WrapCheck()
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
					printf("remove distribution\n");
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