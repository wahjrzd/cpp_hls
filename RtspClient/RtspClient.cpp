#include "RtspClient.h"
#include <iostream>
#include <sstream>
#include <WS2tcpip.h>
#include <vector>
#include "SdpParse.h"

#pragma warning(disable:4996)

RtspClient::RtspClient() :fResponseBuffer(nullptr), fResponseBytesAlreadySeen(0),
fResponseBufferBytesLeft(20000),
m_cli(INVALID_SOCKET),
m_currentCmd("DESCRIBE"),
fSeq(1),
hasVideo(false),
hasAudio(false),
sendVideoSetup(false),
sendAudioSetup(false)
{
	fUserAgent = "User-Agent: Simple RTSP Client\r\n";
	rtp = new RtpUnpacket();
}

RtspClient::~RtspClient()
{
	if (m_dataHandleFuture.valid())
		m_dataHandleFuture.get();

	if (m_cli != INVALID_SOCKET)
		closesocket(m_cli);

	delete fResponseBuffer;

	delete rtp;
}

bool RtspClient::OpenRtsp(const char* url)
{
	std::string user;
	std::string password;
	std::string host;
	unsigned short port;
	parseURL(url, user, password, host, port);
	fBaseUrl = url;
	fResponseBuffer = new char[20000];

	if (!user.empty() && !password.empty())
	{
		fCurrentAuthenticator.setUsernameAndPassword(user.c_str(), password.c_str());
	}

	ConnectServer(host.c_str(), port);

	m_dataHandleFuture = std::async(&RtspClient::WrapHandleData, this);

	SendRequest(m_currentCmd, fBaseUrl);

	return true;
}

int RtspClient::parseURL(const char* url, std::string& usr, std::string& pwd, std::string& ip, unsigned short& port)
{
	char const* prefix = "rtsp://";
	unsigned const prefixLength = 7;
	if (_strnicmp(prefix, url, prefixLength) != 0)
	{
		fprintf(stderr, "URL is not of the form \"%s%s", prefix, "\"");
		return 1;
	}

	// Check whether "<username>[:<password>]@" occurs next.
	unsigned const parseBufferSize = 100;
	char parseBuffer[parseBufferSize];
	char const* from = &url[prefixLength];

	char const* colonPasswordStart = NULL;
	char const* p;
	for (p = from; *p != '\0' && *p != '/'; ++p)
	{
		if (*p == ':' && colonPasswordStart == NULL)
		{
			colonPasswordStart = p;
		}
		else if (*p == '@')
		{
			if (colonPasswordStart == NULL)
				colonPasswordStart = p;

			char const* usernameStart = from;
			unsigned usernameLen = colonPasswordStart - usernameStart;

			usr.assign(usernameStart, usernameLen);

			char const* passwordStart = colonPasswordStart;
			if (passwordStart < p) ++passwordStart; // skip over the ':'
			unsigned passwordLen = p - passwordStart;

			pwd.assign(passwordStart, passwordLen);

			from = p + 1; // skip over the '@'
			break;
		}
	}

	char* to = &parseBuffer[0];
	unsigned i;
	for (i = 0; i < parseBufferSize; ++i)
	{
		if (*from == '\0' || *from == ':' || *from == '/')
		{
			// We've completed parsing the address
			*to = '\0';
			break;
		}
		*to++ = *from++;
	}

	if (i == parseBufferSize)
		return 1;
	
	ip = parseBuffer;
	unsigned short portNum = 554; // default value
	char nextChar = *from;
	if (nextChar == ':')
	{
		sscanf_s(++from, "%hu", &portNum);
		if (portNum < 1 || portNum>65535)
		{
			std::cout << "bad port" << std::endl;
			return 1;
		}
		while (*from >= '0' && *from <= '9') ++from;
	}
	port = portNum;
	const char* urlSuffix = from;
	return 0;
}

int RtspClient::ConnectServer(const char* ip, unsigned short port)
{
	m_cli = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in sa = { AF_INET };
	inet_pton(AF_INET, ip, &sa.sin_addr);
	sa.sin_port = htons(port);
	if (sa.sin_addr.s_addr == INADDR_ANY || sa.sin_addr.s_addr== INADDR_NONE)
	{
		auto host = gethostbyname(ip);
		if (host == NULL || host->h_addr == NULL)
		{
			//
			return 1;
		}
		sa.sin_addr = *(in_addr *)host->h_addr;
	}
	if (connect(m_cli, (sockaddr*)&sa, sizeof(sockaddr_in)) != 0)
	{
		fprintf(stderr, "connect server failed:%d\n", WSAGetLastError());
		return 1;
	}
	return 0;
}

void RtspClient::SendRequest(const std::string cmd, const std::string& url, int irtp, int irtcp)
{
	std::vector<std::string> cv;
	char Header[256];

	memset(Header, 0, 256);
	sprintf_s(Header, "%s %s %s\r\n", cmd.c_str(), url.c_str(), "RTSP/1.0");
	cv.push_back(Header);

	memset(Header, 0, 256);
	sprintf_s(Header, "CSeq: %d\r\n", ++fSeq);
	cv.push_back(Header);

	auto authenticatorStr = fCurrentAuthenticator.createAuthenticatorString(cmd.c_str(), fBaseUrl.c_str());
	if (!authenticatorStr.empty())
		cv.push_back(authenticatorStr);

	cv.push_back(fUserAgent);

	if (cmd == "SETUP")
	{
		if (!sessionID.empty())
		{
			memset(Header, 0, 256);
			sprintf_s(Header, "Session: %s\r\n", sessionID.c_str());
			cv.push_back(Header);
		}

		memset(Header, 0, 256);
		sprintf_s(Header, "Transport: RTP/AVP/TCP;unicast;interleaved=%d-%d\r\n\r\n", irtp, irtcp);
		cv.push_back(Header);
	}
	else if (cmd == "DESCRIBE")
	{
		cv.push_back("Accept: application/sdp\r\n\r\n");
	}
	else if (cmd == "PLAY")
	{
		if (!sessionID.empty())
		{
			memset(Header, 0, 256);
			sprintf_s(Header, "Session: %s\r\n", sessionID.c_str());
			cv.push_back(Header);
		}

		cv.push_back("Range: npt=0.000-\r\n\r\n");
	}
	else if (cmd == "TEARDOWN")
	{
		if (!sessionID.empty())
		{
			memset(Header, 0, 256);
			sprintf_s(Header, "Session: %s\r\n\r\n", sessionID.c_str());
			cv.push_back(Header);
		}
	}

	std::string requestStr;
	auto it = cv.begin();
	while (it != cv.end())
	{
		requestStr.append(*it);
		++it;
	}

	std::cout << requestStr;
	if (::send(m_cli, requestStr.c_str(), requestStr.size(), 0) <= 0)
		fprintf(stderr, "send failed:%d\n", WSAGetLastError());
}

unsigned int RtspClient::WrapHandleData()
{
	fd_set rf;

	while (true)
	{
		FD_ZERO(&rf);
		FD_SET(m_cli, &rf);
		timeval tv = { 2,0 };
		int ret = ::select(0, &rf, nullptr, nullptr, &tv);
		if (ret < 0)
		{
			fprintf(stderr, "select failed:%d\n", WSAGetLastError());
			break;
		}
		else if (ret == 1)
		{
			ret = ::recv(m_cli, &fResponseBuffer[fResponseBytesAlreadySeen], fResponseBufferBytesLeft, 0);
			if (ret <= 0)
			{
				fprintf(stderr, "recv data failed:%d\n", WSAGetLastError());
				break;
			}
			if (fResponseBuffer[0] != 0x24)
			{
				if (HandleCmdData(ret) != 0)
					break;
			}
			else
			{
				fResponseBytesAlreadySeen += ret;
				fResponseBufferBytesLeft -= ret;

				if (HandleRtpData() != 0)
					break;
			}
		}
	}
	return 0;
}

unsigned int RtspClient::HandleCmdData(int newBytesRead)
{
	fResponseBufferBytesLeft -= newBytesRead;
	fResponseBytesAlreadySeen += newBytesRead;
	fResponseBuffer[fResponseBytesAlreadySeen] = '\0';

	unsigned numExtraBytesAfterResponse = 0;
	bool responseSuccess = false; // by default

	do
	{
		// Data was read OK.  Look through the data that we've read so far, to see if it contains <CR><LF><CR><LF>.
		// (If not, wait for more data to arrive.)
		bool endOfHeaders = false;
		char const* ptr = fResponseBuffer;
		if (fResponseBytesAlreadySeen > 3)
		{
			char const* const ptrEnd = &fResponseBuffer[fResponseBytesAlreadySeen - 3];
			while (ptr < ptrEnd)
			{
				if (*ptr++ == '\r' && *ptr++ == '\n' && *ptr++ == '\r' && *ptr++ == '\n') 
				{
					// This is it
					endOfHeaders = true;
					break;
				}
			}
		}

		if (!endOfHeaders) return 0; // subsequent reads will be needed to get the complete response
		std::cout << fResponseBuffer;
		ResponseInfo resp;
		ParseBuf(fResponseBuffer, &resp);

		if (resp.responseCode == 401)
		{
			if (!resp.headers["WWW-Authenticate"].empty())
			{
				std::string realm,nonce;
			
				parseWWWAuth(resp.headers["WWW-Authenticate"], realm, nonce);

				SendRequest(m_currentCmd, fBaseUrl);
			}
			else
				std::cout << "WWW-Authenticate empty?" << std::endl;
		}
		else if (resp.responseCode == 200)
		{
			if (!resp.headers["Content-Base"].empty())
			{
				fBaseUrl = resp.headers["Content-Base"];
			}
			if (!resp.headers["Content-Length"].empty())
			{
				SdpParse sdp;
				sdp.parse(resp.content);//fVideoControlPath
				Media m;
				if (sdp.GetMedia("video", m))
				{
					fVideoControlPath = m.Attributes["control"];
					auto rtpmap = m.Attributes["rtpmap"];
					char p1[32];
					char p2[32];
					char p3[32];
					if (sscanf(rtpmap.c_str(), "%[0-9] %[^/]/%[0-9]", p1, p2, p3) == 3)
						rtp->SetVideoCodecType(p2);

					hasVideo = true;
				}
				if (sdp.GetMedia("audio", m))
				{
					fAudioControlPath = m.Attributes["control"];
					auto rtpmap = m.Attributes["rtpmap"];
					
					char p1[32];
					char p2[32];
					char p3[32];
					if (sscanf(rtpmap.c_str(), "%[0-9] %[^/]/%[0-9]", p1, p2, p3) == 3)
						rtp->SetAudioCodecType(p2);

					hasAudio = true;
				}
			}
			if (!resp.headers["Session"].empty())
			{
				sessionID = resp.headers["Session"];
			}

			if (m_currentCmd == "DESCRIBE")
			{
				m_currentCmd = "SETUP";
				if (hasVideo && !sendVideoSetup)
				{
					std::string tempUrl = fBaseUrl;
					if (!fVideoControlPath.empty())
					{
						if (fBaseUrl.back() != '/')
							tempUrl += "/";

						if (_strnicmp(fVideoControlPath.c_str(), "rtsp:", 5) == 0)//full path
							tempUrl = fVideoControlPath;
						else
							tempUrl += fVideoControlPath;

					}
					SendRequest(m_currentCmd, tempUrl, vRTP, vRTCP);
					sendVideoSetup = true;
				}
				else if (hasAudio && !sendAudioSetup)
				{
					std::string tempUrl = fBaseUrl;
					if (!fAudioControlPath.empty())
					{
						if (fBaseUrl.back() != '/')
							tempUrl += "/";

						if (_strnicmp(fAudioControlPath.c_str(), "rtsp:", 5) == 0)//full path
							tempUrl = fAudioControlPath;
						else
							tempUrl += fAudioControlPath;
					}

					SendRequest(m_currentCmd, tempUrl, aRTP, aRTCP);
					sendAudioSetup = true;
				}
				else
					std::cout << "TODO" << std::endl;
			}
			else if (m_currentCmd == "SETUP")
			{
				if (hasAudio && !sendAudioSetup)
				{
					m_currentCmd = "SETUP";
					std::string tempUrl = fBaseUrl;
					if (!fAudioControlPath.empty())
					{
						if (fBaseUrl.back() != '/')
							tempUrl += "/";

						if (_strnicmp(fAudioControlPath.c_str(), "rtsp:", 5) == 0)//full path
							tempUrl = fAudioControlPath;
						else
							tempUrl += fAudioControlPath;
					}

					SendRequest(m_currentCmd, tempUrl, aRTP, aRTCP);
					sendAudioSetup = true;
				}
				else
				{
					m_currentCmd = "PLAY";
					SendRequest(m_currentCmd, fBaseUrl);
				}
			}
			else if (m_currentCmd == "TEARDOWN")
				return 1;
			
		}
		else
		{
			//TODO
		}
		
	} while (0);

	fResponseBytesAlreadySeen = 0;
	fResponseBufferBytesLeft = 20000;
	return 0;
}

void RtspClient::SetRawCallback(RawCallback cb, void* arg)
{
	if (rtp)
		rtp->SetRawCallback(cb, arg);
}

void RtspClient::ParseBuf(const char* buf, ResponseInfo* resp)
{
	std::stringstream sss(buf);
	std::string line;
	std::vector<std::string> lines;
	while (true)
	{
		auto& stream = std::getline(sss, line);
		line.pop_back();
		if (line.empty())
		{
			if (!stream.eof())
			{
				char* p = new char[1024];
				auto sz = stream.read(p, 1024).gcount();
				resp->content.append(p, sz);
				delete p;
			}
			break;
		}
		lines.push_back(line);
	}

	std::string protolStr;
	unsigned responseCode = 200;
	std::string responseString = "OK";
	auto pos = lines[0].find(' ');
	if (pos != lines[0].npos)
	{
		resp->protocol = lines[0].substr(0, pos);
		resp->responseCode = std::stoi(std::string(lines[0].c_str() + pos + 1, 3));
		resp->responseString.append(lines[0].c_str() + pos + 1 + 3 + 1);
	}

	for (size_t i = 1; i < lines.size(); i++)
	{
		auto pos = lines[i].find(":");
		if (pos != lines[i].npos)
		{
			resp->headers.insert({ lines[i].substr(0, pos) ,lines[i].substr(pos + 2) });
		}
	}
}

void RtspClient::parseWWWAuth(const std::string& str, std::string& _realm, std::string& _nonce)
{
	int i = 0;
	char* realm = new char[str.size()];
	char* nonce = new char[str.size()];
	char* stale = new char[str.size()];
	if (sscanf(str.c_str(), "Digest realm=\"%[^\"]\", nonce=\"%[^\"]\", stale=%[a-zA-Z]", realm, nonce, stale) == 3)
	{
		fCurrentAuthenticator.setRealmAndNonce(realm, nonce);
	}
	else if (sscanf(str.c_str(), "Digest realm=\"%[^\"]\", nonce=\"%[^\"]\", stale=%[a-zA-Z]", realm, nonce, stale) == 2)
	{
		fCurrentAuthenticator.setRealmAndNonce(realm, nonce);
	}
	else if (sscanf(str.c_str(), "Basic realm=\"%[^\"]\"", realm) == 1)
	{
		fCurrentAuthenticator.setRealmAndNonce(realm, NULL);
	}
	delete realm;
	delete nonce;
	delete stale;
}

unsigned int RtspClient::HandleRtpData()
{
	if (fResponseBuffer[0] != 0x24)//maybe a cmd
	{
		return HandleCmdData(0);
	}

	int nChannel = fResponseBuffer[1];
	auto a = (unsigned char)fResponseBuffer[2];
	auto b = (unsigned char)fResponseBuffer[3];
	unsigned short sz = (a << 8) | b;

	if (sz + 4 > fResponseBytesAlreadySeen)//less
	{
		return 0;
	}
	
	if (nChannel == vRTP)
	{
		unsigned char* p = (unsigned char*)fResponseBuffer + 4;

		rtp->InputRtpData(p, sz, "video");
	}
	else if (nChannel == vRTCP)
	{
		//std::cout << "rtcp data" << std::endl;
	}
	else if (nChannel == aRTP)
	{
		unsigned char* p = (unsigned char*)fResponseBuffer + 4;
		rtp->InputRtpData(p, sz, "audio");
	}
	else if (nChannel == aRTCP)
	{

	}
	else
		std::cout << "TODO" << std::endl;

	if (sz + 4 < fResponseBytesAlreadySeen)//more
	{
		auto extraLength = fResponseBytesAlreadySeen - (sz + 4);

		char* end = &fResponseBuffer[sz + 4];
		memmove(fResponseBuffer, end, extraLength);
		fResponseBytesAlreadySeen -= sz + 4;
		fResponseBufferBytesLeft += sz + 4;

		HandleRtpData();
	}
	else
	{
		fResponseBytesAlreadySeen = 0;
		fResponseBufferBytesLeft = 20000;
	}

	return 0;
}

void RtspClient::Stop()
{
	if (m_cli != INVALID_SOCKET)
	{
		m_currentCmd = "TEARDOWN";
		SendRequest(m_currentCmd, fBaseUrl);
	}
}