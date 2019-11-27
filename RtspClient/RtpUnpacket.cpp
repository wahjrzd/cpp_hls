#include "RtpUnpacket.h"
#include <iostream>

RtpUnpacket::RtpUnpacket() : naluType(0),
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
		else if (_strcmpi(m_videoCodec.c_str(), "H265") == 0)
			ParseHEVCRTP(data + 12, sz - 12, timeStamp, mark == 0 ? false : true);
	}
	else if (type == "audio")
	{
		if (m_audioCodec == "PCMU" || m_audioCodec == "PCMA")
			ParseG711RTP(data + 12, sz - 12, timeStamp, mark == 0 ? false : true);
		else if (_strcmpi(m_audioCodec.c_str(), "MPEG4-GENERIC") == 0)
			ParseAACRTP(data + 12, sz - 12, timeStamp, mark == 0 ? false : true);
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
			ff.data = frameData;
			ff.timeStamp = timeStamp / 90;
			ff.samplingRate = m_videoSampleRate;
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
			ff.codecType = m_audioCodec;
			ff.data = frameData;
			ff.timeStamp = timeStamp / (m_audioSampleRate / 1000);
			ff.samplingRate = m_audioSampleRate;

			m_cb(ff, pUsr);
		}
		frameData.clear();
	}
		
	return 0;
}

static unsigned const samplingFrequencyFromIndex[16] = {
  96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
  16000, 12000, 11025, 8000, 7350, 0, 0, 0
};


int RtpUnpacket::ParseAACRTP(unsigned char* data, unsigned short sz, unsigned int timeStamp, bool mark)
{
	frameData.append(data, sz);
	if (mark)
	{
		if (m_cb)
		{
			//adts header               bits       
			//syncword 12                 12 FFF
			//id                           1 0:mpeg4 1:mpeg2
			//layer                        2 00
			//protection_absend            1 1(no crc) 0(crc)  fff1
			//profile                      2 01
			//sampling_frequecny_index     4 1000 
			//private_bit                  1 0
			//channel_configuration        3 1   60
			//original_copy                1 0
			//home                         1 0
			//copyright                    1 0
			//copyright start              1 0   40
			//framelength                 13 
			//buffer fullness             11 11111111111
			//number of aacframe minus 1   2 00
			//https://wiki.multimedia.cx/index.php?title=ADTS
			unsigned char ADTS[] = { 0xFF, 0xF1, 0x00, 0x00, 0x00, 0x00, 0xFC };
			switch (m_audioSampleRate)
			{
			case 16000:
				ADTS[2] = 0x60; break;//main profile(1)  sampling_frequecny_index 8
			case 32000:
				ADTS[2] = 0x54; break;
			case 48000:
				ADTS[2] = 0x4c; break;
			case 64000:
				ADTS[2] = 0x48; break;
			default:
				break;
			}
		
			ADTS[3] = (m_soundTrack == 2) ? 0x80 : 0x40;//soundtrack 1
			int len = sz - 2 + 7;
			//                                     4     len 5      6 
			ADTS[4] = (len & 0xfff8) >> 3;//00 1111 1111 111 1 1111 1111 1100
			ADTS[5] = (len << 5) | 0x1F;//µÍÈýÎ»

			if (data[0] != 0x00 && data[1] != 0x10)
				std::cerr << "correct aac stream" << std::endl;
			
			FrameInfo ff;
			ff.mediaType = "audio";
			ff.codecType = m_audioCodec;
			ff.data.append(ADTS, 7);
			ff.data.append(frameData.c_str() + 2, frameData.size() - 2);
			ff.timeStamp = timeStamp;

			m_cb(ff, pUsr);
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