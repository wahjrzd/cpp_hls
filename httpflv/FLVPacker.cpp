#include "FLVPacker.h"

FLVPacker::FLVPacker() :m_cb(nullptr)
{

}

FLVPacker::~FLVPacker()
{

}

void FLVPacker::deliverVideoESPacket(unsigned char const* frame, unsigned int frame_size, unsigned int pts, bool iFrame)
{

}

void FLVPacker::deliverAudioESPacket(unsigned char const* frame, unsigned int frame_size, unsigned int pts)
{

}

void FLVPacker::GenerateFLVTagHeader(FLV_TAG_TYPE t, unsigned int bodySize, unsigned int timeStamp)
{

}

std::basic_string<std::uint8_t> FLVPacker::GenerateAVCSequenceHeader(const std::basic_string<std::uint8_t>& sps, const std::basic_string<std::uint8_t>& pps)
{
	std::basic_string<std::uint8_t> s;
	unsigned char a[11];
	a[0] = 0x17;
	a[1] = 0;//0 avcsequence
	a[2] = 0;
	a[3] = 0;
	a[4] = 0;//compositionTime 
	a[5] = 1;//version
	a[6] = sps[1];
	a[7] = sps[2];
	a[8] = sps[3];
	a[9] = 0xff;//6 bits reserved(111111) + 2 bits nal size length - 1 (11)
	a[10] = 0xE1;//3 bits reserved (111) + 5 bits number of sps (00001)
	s.append(a, 10);

	a[0] = (sps.size() >> 8) & 0xff;
	a[1] = sps.size() & 0xff;
	s.append(a, 2);
	s.append(sps);

	a[0] = 0x01;
	a[1] = (pps.size() >> 8) & 0xff;
	a[2] = pps.size() & 0xff;
	s.append(a, 3);
	s.append(pps);
	return s;
}