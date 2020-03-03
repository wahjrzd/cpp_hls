#include "RTCPUnpacket.h"
#include <Windows.h>
#include "RtpUnpacket.h"

RTCPUnpacket::RTCPUnpacket(RtpUnpacket* _rtp) :rtp(_rtp)
{
	DWORD sz;
	char computerName[64];
	GetComputerNameA(computerName, &sz);
	
	//header(4)+ssrc(4)+type(1)+length(1)+text(...)+type(end)
	sdec_len = 4 + (4 + 1 + 1 + sz + 1 + 3) / 4 * 4;
	sdec = new unsigned char[sdec_len];
	memset(sdec, 0, sdec_len);
	sdec[0] = 0x81;//
	sdec[1] = 202;//Packet type
	sdec[2] = (((sdec_len - 4) >> 2) >> 8) & 0xFF;//
	sdec[3] = (sdec_len - 4) >> 2;

	sdec[8] = 1;
	sdec[9] = sz;
	memcpy(sdec + 10, computerName, sz);
}

RTCPUnpacket::~RTCPUnpacket()
{
	delete sdec;
}

int RTCPUnpacket::InputRTCPData(unsigned char* data, unsigned short sz, unsigned char t)
{
	auto v = (data[0] >> 6) & 0x03;//version
	auto p = (data[0] >> 5) & 0x01;//padding
	auto rc = data[0] & 0x1f;//reception report count
	auto pt = data[1];//packet type
	auto len = (data[2] << 8) | data[3];//Length
	switch (pt)
	{
	case 200://RTCP_SR
	{
		if (t == 0)
		{
			video_sr_ts = rtpclock();
			video_last_sr = (data[10] << 24) | (data[11] << 16) | (data[12] << 8) | data[13];
		}
		else if (t == 1)
		{
			audio_sr_ts = rtpclock();
			audio_last_sr = (data[10] << 24) | (data[11] << 16) | (data[12] << 8) | data[13];
		}

		ParseSR(data + 4);
		//sdec
		v = (data[28] >> 6) & 0x03;
		p = (data[28] >> 5) & 0x01;
		rc = data[28] & 0x1f;
		pt = data[29];
		len = (data[30] << 8) | data[31];
		
		auto ssrc = (data[32] << 24) | (data[33] << 16) | (data[34] << 8) | data[35];
		for (int i = 0; i < rc; i++)
		{
			//RTCP_SDES_CNAME 1
			//RTCP_SDES_NAME 2
			//...
			auto type = data[36];//Type
			auto length = data[37];
			auto text = data + 38;
			//...
		}
	}break;
	case 201://RTCP_RR
		break;
	case 202://RTCP_SDES
		break;
	case 203://RTCP_BYE
	{
		auto ssrc = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
	}break;
	case 204://RTCP_APP
		break;
	default:
		//TODO
		break;
	}
	return 0;
}

void RTCPUnpacket::ParseSR(unsigned char* data)
{
	unsigned int ssrc = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
	unsigned int ntpmsw = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
	unsigned int ntplsw = (data[8] << 24) | (data[9] << 16) | (data[10] << 8) | data[11];
	unsigned int rtpts = (data[12] << 24) | (data[13] << 16) | (data[14] << 8) | data[15];
	unsigned int spc = (data[16] << 24) | (data[17] << 16) | (data[18] << 8) | data[19];
	unsigned int soc = (data[20] << 24) | (data[21] << 16) | (data[22] << 8) | data[23];
}

void RTCPUnpacket::ParseRR(unsigned char* data)
{
	
}

std::basic_string<unsigned char> RTCPUnpacket::PackRR(unsigned char m)
{
	auto p = temp;
	*p++ = 0x81;
	*p++ = 201;//RTCP_RR
	*p++ = 0;
	*p++ = 7;//length
	if (m == 0)//report ssrc && sender ssrc
	{
		*p++ = (videoSendSSRC >> 24) & 0xFF;
		*p++ = (videoSendSSRC >> 16) & 0xFF;
		*p++ = (videoSendSSRC >> 8) & 0xFF;
		*p++ = videoSendSSRC & 0xFF;

		*p++ = (videoRecvSSRC >> 24) & 0xFF;
		*p++ = (videoRecvSSRC >> 16) & 0xFF;
		*p++ = (videoRecvSSRC >> 8) & 0xFF;
		*p++ = videoRecvSSRC & 0xFF;

		*p++ = 0;//fraction lost
		*p++ = 0xFF;//cumulative number
		*p++ = 0xFF;
		*p++ = 0xFF;

		unsigned int extended_seq_num = rtp->lastVideoSeq + 65536 * rtp->videoCycleCount;
		*p++ = (extended_seq_num >> 24) & 0xFF;
		*p++ = (extended_seq_num >> 16) & 0xFF;
		*p++ = (extended_seq_num >> 8) & 0xFF;
		*p++ = extended_seq_num & 0xFF;
		//jitter
		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
		*p++ = 0xB8;
		//last sr timestamp
		*p++ = video_last_sr >> 24;
		*p++ = video_last_sr >> 16;
		*p++ = video_last_sr >> 8;
		*p++ = video_last_sr;
		//delay
		unsigned long long delayTime = (rtpclock() - video_sr_ts) * 65536 / 1000;
		*p++ = (delayTime >> 24) & 0xFF;
		*p++ = (delayTime >> 16) & 0xFF;
		*p++ = (delayTime >> 8) & 0xFF;
		*p++ = delayTime & 0xFF;
	}
	else if (m == 1)
	{
		*p++ = (audioSendSSRC >> 24) & 0xFF;
		*p++ = (audioSendSSRC >> 16) & 0xFF;
		*p++ = (audioSendSSRC >> 8) & 0xFF;
		*p++ = audioSendSSRC & 0xFF;

		*p++ = (audioRecvSSRC >> 24) & 0xFF;
		*p++ = (audioRecvSSRC >> 16) & 0xFF;
		*p++ = (audioRecvSSRC >> 8) & 0xFF;
		*p++ = audioRecvSSRC & 0xFF;

		*p++ = 0;//fraction lost
		*p++ = 0xFF;//cumulative number
		*p++ = 0xFF;
		*p++ = 0xFF;

		unsigned int extended_seq_num = rtp->lastAudioSeq + 65536 * rtp->audioCycleCount;
		*p++ = (extended_seq_num >> 24) & 0xFF;
		*p++ = (extended_seq_num >> 16) & 0xFF;
		*p++ = (extended_seq_num >> 8) & 0xFF;
		*p++ = extended_seq_num & 0xFF;
		//jitter
		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
		*p++ = 0xB8;
		//last sr timestamp
		*p++ = audio_last_sr >> 24;
		*p++ = audio_last_sr >> 16;
		*p++ = audio_last_sr >> 8;
		*p++ = audio_last_sr;
		//delay
		unsigned long long delayTime = (rtpclock() - audio_sr_ts) * 65536 / 1000;
		*p++ = (delayTime >> 24) & 0xFF;
		*p++ = (delayTime >> 16) & 0xFF;
		*p++ = (delayTime >> 8) & 0xFF;
		*p++ = delayTime & 0xFF;
	}

	//sdec
	if (m == 0)
	{
		sdec[4] = videoRecvSSRC >> 24;
		sdec[5] = videoRecvSSRC >> 16;
		sdec[6] = videoRecvSSRC >> 8;
		sdec[7] = videoRecvSSRC;
	}
	else
	{
		sdec[4] = audioRecvSSRC >> 24;
		sdec[5] = audioRecvSSRC >> 16;
		sdec[6] = audioRecvSSRC >> 8;
		sdec[7] = audioRecvSSRC;
	}
	memcpy(p, sdec, sdec_len);
	tempStr.assign(temp, p - temp + sdec_len);
	return tempStr;
}

unsigned long long RTCPUnpacket::rtpclock()
{
	//from 1970/1/1 to now ms
	FILETIME ft;
	GetSystemTimeAsFileTime((FILETIME*)&ft);
	LARGE_INTEGER lt;
	lt.HighPart = ft.dwHighDateTime;
	lt.LowPart = ft.dwLowDateTime;
	return lt.QuadPart / 10000 - 0xA9730B66800;
}

unsigned long long RTCPUnpacket::ntpclock(unsigned long long clock)
{
	unsigned long long ntp;
	ntp = ((clock / 1000) + 0x83AA7E80) << 32;//高32位 1900/1/1 到现在的秒数
	//低32位 微秒数的4294.967296(=2^32/10^6)倍
	// low 32 bits in picosecond
	// ms * 2^32 / 10^3
	// 10^6 = 2^6 * 15625
	// => ms * 1000 * 2^26 / 15625
	ntp |= (unsigned int)(((clock % 1000) * 1000) * 0x4000000 / 15625);

	return ntp;
}