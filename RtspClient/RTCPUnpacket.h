#pragma once
#include <string>

class RtspClient;
class RtpUnpacket;

class RTCPUnpacket
{
public:
	RTCPUnpacket(RtpUnpacket* _rtp);
	~RTCPUnpacket();

	int InputRTCPData(unsigned char* data, unsigned short sz, unsigned char t);
	//0-video 1 audio
	std::basic_string<unsigned char> PackRR(unsigned char m);
private:
	void ParseSR(unsigned char* data);

	void ParseRR(unsigned char* data);

	unsigned long long rtpclock();

	unsigned long long ntpclock(unsigned long long clock);
private:
	unsigned char temp[256];
	std::basic_string<unsigned char> tempStr;
	
	unsigned int videoRecvSSRC = 0;
	unsigned int audioRecvSSRC = 1;
	unsigned int videoSendSSRC = 2;
	unsigned int audioSendSSRC = 3;

	unsigned long long video_sr_ts;//sr recv timestamp
	unsigned long long audio_sr_ts;//sr recv timestamp

	unsigned int video_last_sr = 0;//Last sr timestamp
	unsigned int audio_last_sr = 0;//Last sr timestamp
	unsigned char* sdec;
	size_t sdec_len = 0;

	RtpUnpacket* rtp;
	friend RtspClient;
};

