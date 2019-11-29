#pragma once
#include <string>
#include <vector>

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
	std::basic_string<std::uint8_t> data;
	std::basic_string<std::uint8_t> videoSequence;
	std::basic_string<std::uint8_t> audioSequence;
	std::basic_string<std::uint8_t> meta;
	FLVFramePacket()
	{
		
	}
};

typedef unsigned(*FLVCallback)(FLVFramePacket& f, void* arg);

const std::uint8_t FLV_FILE_HEADER[] = {
	'F','L','V',0x01,
	0x05,//00000 A 0 V
	0x00,0x00,0x00,0x09,//dataoffset
	0x00,0x00,0x00,0x00//previous size
};

class FLVPacker
{
public:
	FLVPacker();
	~FLVPacker();

	void deliverVideoESPacket(const std::basic_string<std::uint8_t>& frame, unsigned int pts, bool iFrame);

	void deliverAudioESPacket(const std::basic_string<std::uint8_t>& frame, unsigned int pts);

	void SetCallback(FLVCallback cb, void* arg)
	{
		m_cb = cb;
		m_arg = arg;
	}
private:
	std::basic_string<std::uint8_t> GenerateFLVTagHeader(FLV_TAG_TYPE t, unsigned int bodySize, unsigned int timeStamp);

	std::basic_string<std::uint8_t> GenerateAVCSequenceHeader(const std::basic_string<std::uint8_t>& sps, const std::basic_string<std::uint8_t>& pps);

	std::basic_string<std::uint8_t> PackNalus(std::vector<std::basic_string<std::uint8_t>>& vn, bool iFrame);

	std::basic_string<std::uint8_t> GenerateVideoFVLTag(std::vector<std::basic_string<std::uint8_t>>& nalus, unsigned int timeStamp, bool iFrame);

	std::basic_string<std::uint8_t> GenerateAudioTagHead();

	std::basic_string<std::uint8_t> onMeta(double width, double heigth, double frameRate);

	void GetNalus(const std::basic_string<std::uint8_t>& data, std::vector<std::basic_string<std::uint8_t>>& vn);
private:
	std::basic_string<std::uint8_t> m_videoSequenceHeader;
	std::basic_string<std::uint8_t> m_audioSequenceHeader;
	std::basic_string<std::uint8_t> m_meta;
	unsigned int pppss = 0;
	FLVCallback m_cb;
	void* m_arg;
};