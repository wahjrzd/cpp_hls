#pragma once
#include <string>

//PCMU g711u
//PCMA g711a
//AAC MPEG4-Generic

struct FrameInfo
{
	std::string mediaType;
	unsigned char frameType;
	std::basic_string<unsigned char> data;
	unsigned int timeStamp;
	int reserver;
	FrameInfo()
	{
		frameType = 0;
	}
};

typedef unsigned(*RawCallback)(FrameInfo& f, void* arg);

class RtpUnpacket
{
public:
	RtpUnpacket();
	~RtpUnpacket();

	int InputRtpData(unsigned char* data, unsigned short sz, const std::string& type);

	void SetRawCallback(RawCallback cb, void* usr);

	void SetVideoCodecType(const std::string& _type)
	{
		m_videoCodec = _type;
	}

	void SetAudioCodecType(const std::string& _type)
	{
		m_audioCodec = _type;
	}
private:
	
	int ParseAVCRTP(unsigned char* data, unsigned short sz, unsigned int timeStamp, bool mark);
	int ParseHEVCRTP(unsigned char* data, unsigned short sz, unsigned int timeStamp, bool mark);

	int ParseG711RTP(unsigned char* data, unsigned short sz, unsigned int timeStamp, bool mark);
	int ParseAACRTP(unsigned char* data, unsigned short sz, unsigned int timeStamp, bool mark);
private:
	std::string m_videoCodec;
	std::string m_audioCodec;
	unsigned int t;
	std::basic_string<unsigned char> frameData;
	unsigned char naluType;
private:
	void* pUsr;
	RawCallback m_cb;
};

