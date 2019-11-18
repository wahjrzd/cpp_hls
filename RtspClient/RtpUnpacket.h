#pragma once
#include <string>

struct FrameInfo
{
	std::string mediaType;
	unsigned char frameType;
	std::basic_string<unsigned char> data;
	unsigned int timeStamp;
	int reserver;
};

typedef unsigned(*RawCallback)(FrameInfo& f, void* arg);

class RtpUnpacket
{
public:
	RtpUnpacket();
	~RtpUnpacket();

	int InputRtpData(unsigned char* data, unsigned short  sz);
	void SetRawCallback(RawCallback cb, void* usr);
private:
	int ParseAVCRTP(unsigned char* data, unsigned short  sz);
private:
	unsigned int t;
	std::basic_string<unsigned char> frameData;
	unsigned char naluType;
private:
	void* pUsr;
	RawCallback m_cb;
};

