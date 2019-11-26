#include "RtpUnpacket.h"
#include <iostream>

RtpUnpacket::RtpUnpacket() :t(0), naluType(0),
pUsr(nullptr), m_cb(nullptr),
m_videoCodec("H264"),
m_audioCodec("PCMU")
{

}

RtpUnpacket::~RtpUnpacket()
{
	pUsr = nullptr;
	m_cb = nullptr;
}

int RtpUnpacket::InputRtpData(unsigned char* data, unsigned short sz, const std::string& type)
{
	auto cc = (data[0] & 0x10);
	auto mark = (data[1] & 0x80) >> 7;
	auto pt = data[1] & 0x7f;

	unsigned short seq = (data[2] << 8) | data[3];
	unsigned int timeStamp = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
	unsigned int ssrc = (data[8] << 24) | (data[9] << 16) | (data[10] << 8) | data[11];

	if (type == "video")
	{
		if (m_videoCodec == "H264")
			ParseAVCRTP(data + 12, sz - 12, timeStamp, mark == 0 ? false : true);
		else if (m_videoCodec == "H265")
			ParseHEVCRTP(data + 12, sz - 12, timeStamp, mark == 0 ? false : true);
	}
	else if (type == "audio")
	{
		if (m_audioCodec == "PCMU" || m_audioCodec == "PCMA")
		{
			ParseG711RTP(data + 12, sz - 12, timeStamp, mark == 0 ? false : true);
		}
		else if (m_audioCodec == "MPEG4-GENERIC")
		{
			ParseAACRTP(data + 12, sz - 12, timeStamp, mark == 0 ? false : true);
		}
		else
		{
			//TODO
		}
	}
	else
	{
		//TODO
	}
	return 0;
}

int RtpUnpacket::ParseAVCRTP(unsigned char* data, unsigned short sz, unsigned int timeStamp, bool mark)
{
	if ((data[0] & 0x1c) == 0x1c)/*FUA*/
	{
		auto se = data[1] >> 6;
		if (se == 2)/*s bit*/
		{
			unsigned char nal[] = { 0x00,0x00,0x00,0x01 };
			frameData.append(nal, 4);
			unsigned char x = (data[0] & 0xE0) | (data[1] & 0x1F);
			naluType = (data[1] & 0x1F);
			frameData.push_back(x);
			frameData.append(data + 2, sz - 2);
		}
		else if (se != 3)/*e or 0 bit*/
		{
			frameData.append(data + 2, sz - 2);
		}
		else
			std::cout << "error" << std::endl;
	}
	else
	{
		naluType = (data[0] & 0x1f);
		unsigned char nal[] = { 0x00,0x00,0x00,0x01 };
		frameData.append(nal, 4);
		frameData.append(data, sz);
	}

	if (mark)
	{
		if (m_cb)
		{
			FrameInfo ff;
			ff.mediaType = "video";
			ff.data = frameData;
			ff.timeStamp = timeStamp / 90;
			ff.frameType = naluType;

			m_cb(ff, pUsr);
		}
		frameData.clear();
	}
	
	return 0;
}

int RtpUnpacket::ParseHEVCRTP(unsigned char* data, unsigned short sz, unsigned int timeStamp, bool mark)
{
	return 0;
}

int RtpUnpacket::ParseG711RTP(unsigned char* data, unsigned short sz, unsigned int timeStamp, bool mark)
{
	frameData.append(data, sz);

	if (mark)
	{
		if (m_cb)
		{
			FrameInfo ff;
			ff.mediaType = "audio";
			ff.data = frameData;
			ff.timeStamp = timeStamp / 8;

			m_cb(ff, pUsr);
		}
		frameData.clear();
	}
		
	return 0;
}

int RtpUnpacket::ParseAACRTP(unsigned char* data, unsigned short sz, unsigned int timeStamp, bool mark)
{
	frameData.append(data, sz);
	if (mark)
	{
		if (m_cb)
		{
			auto a = data[0];
			auto b = data[1];
			auto c = data[2];
			auto d = data[3];
		}
		frameData.clear();
	}
	return 0;
}

void RtpUnpacket::SetRawCallback(RawCallback cb, void* usr)
{
	m_cb = cb;
	pUsr = usr;
}