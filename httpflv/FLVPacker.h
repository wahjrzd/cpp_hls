#pragma once
#include <string>

enum class FLV_TAG_TYPE
{
	FLV_TAG_TYPE_AUDIO = 0x08,
	FLV_TAG_TYPE_VIDEO = 0x09,
	FLV_TAG_TYPE_META = 0x12
};

enum class FLV_FRAME
{
	FLV_FRAME_KEY = 1,
	FLV_FRAME_INTER = 2
};

enum class FLV_CODEC
{
	FLV_CODECID_H264 = 7,
	FLV_CODECID_MP3 = 2 << 4,
	FLV_CODECID_AAC = 10 << 4
};

struct FLVFramePacket
{
	unsigned char fileHeader[13];
	std::basic_string<unsigned char> data;
	FLVFramePacket()
	{
		fileHeader[0] = 'F';
		fileHeader[1] = 'L';
		fileHeader[2] = 'V';
		fileHeader[3] = 1;//version
		fileHeader[4] = 0x05;//00000 A 0 V
		//dataoffset
		fileHeader[5] = 0;
		fileHeader[6] = 0;
		fileHeader[7] = 0;
		fileHeader[8] = 9;
		//previous size
		fileHeader[9] = 0;
		fileHeader[10] = 0;
		fileHeader[11] = 0;
		fileHeader[12] = 0;
	}
};

typedef unsigned(*FLVCallback)(FLVFramePacket& f, void* arg);

class FLVPacker
{
public:
	FLVPacker();
	~FLVPacker();

	void deliverVideoESPacket(unsigned char const* frame, unsigned int frame_size, unsigned int pts, bool iFrame);

	void deliverAudioESPacket(unsigned char const* frame, unsigned int frame_size, unsigned int pts);

	void SetCallback(FLVCallback cb, void* arg)
	{
		m_cb = cb;
		m_arg = arg;
	}
private:
	void GenerateFLVTagHeader(FLV_TAG_TYPE t, unsigned int bodySize, unsigned int timeStamp);
	std::basic_string<std::uint8_t> GenerateAVCSequenceHeader(const std::basic_string<std::uint8_t>& sps, const std::basic_string<std::uint8_t>& pps);
	
private:
	FLVCallback m_cb;
	void* m_arg;
};