#pragma once
#include <WinSock2.h>
#include "Authenticator.h"
#include <map>
#include "RtpUnpacket.h"

struct ResponseInfo
{
	std::string protocol;
	unsigned responseCode;
	std::string responseString;
	std::map<std::string, std::string> headers;
	std::string content;
};

class RtspClient
{
public:
	RtspClient();
	~RtspClient();

	bool OpenRtsp(const char* url);
	void SetRawCallback(RawCallback cb, void* arg);
	void Stop();
private:
	int parseURL(const char* url, std::string& usr, std::string& pwd, std::string& ip, unsigned short& port);

	int ConnectServer(const char* ip, unsigned short port);

	void SendRequest(const std::string cmd, const std::string& url, int irtp = 0, int irtcp = 1);

	void ParseBuf(const char* buf, ResponseInfo* __out resp);

	void parseWWWAuth(const std::string& str, std::string& _realm, std::string& _nonce);

	static unsigned int __stdcall StaticHandleData(void* arg);
	unsigned int WrapHandleData();

	unsigned int HandleCmdData(int newBytesRead);
	unsigned int HandleRtpData();
private:
	std::string m_currentCmd;
	std::string fBaseUrl;

	bool hasVideo;
	bool sendVideoSetup;
	bool hasAudio;
	bool sendAudioSetup;
	std::string fVideoControlPath;
	std::string fAudioControlPath;
	int vRTP = 0;
	int vRTCP = 1;
	int aRTP = 2;
	int aRTCP = 3;

	std::string fUserAgent;
	std::string sessionID;
	char* fResponseBuffer;
	unsigned fResponseBytesAlreadySeen, fResponseBufferBytesLeft;
	unsigned fSeq;

	Authenticator fCurrentAuthenticator;

	RtpUnpacket* rtp;
private:
	SOCKET m_cli;
	HANDLE m_handleThr;
};

