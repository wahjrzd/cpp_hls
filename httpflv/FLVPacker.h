#pragma once
#include <string>
#include <vector>
#include <functional>

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
	std::string type;
	std::function<std::basic_string<std::uint8_t>(void*)> GetCodecInfo;

	void* arg;
	FLVFramePacket() :arg(nullptr)
	{
		type = "video";
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
	std::basic_string<std::uint8_t> CodecInfo();

	std::basic_string<std::uint8_t> videoSequenceTag();

	std::basic_string<std::uint8_t> audioSequenceTag();

	std::basic_string<std::uint8_t> onMeta(double width, double heigth, double frameRate);

	void GetNalus(const std::basic_string<std::uint8_t>& data, std::vector<std::pair<size_t, size_t>>& vo);

	unsigned char* FLVTagHead(unsigned char* in, FLV_TAG_TYPE t, size_t bodySize, size_t timeStamp);

	unsigned char* PackNalus(unsigned char* in, const std::basic_string<std::uint8_t>& data, std::vector<std::pair<size_t, size_t>>& vo, bool iFrame);
private:

	std::basic_string<std::uint8_t> m_meta;
	unsigned int m_startVideoPts = 0;
	unsigned int m_startAudioPts = 0;

	std::basic_string<std::uint8_t> m_sps;
	std::basic_string<std::uint8_t> m_pps;

	FLVCallback m_cb;
	void* m_arg;

	unsigned int m_maxBufferSize;
	unsigned char* m_buf;
};