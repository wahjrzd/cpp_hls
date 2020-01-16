#include "FLVPacker.h"
#include "AMF.h"

FLVPacker::FLVPacker() :m_cb(nullptr), m_maxBufferSize(1024 * 200)
{
	m_meta = onMeta(1920, 1080, 25);

	m_buf = new unsigned char[m_maxBufferSize];
}

FLVPacker::~FLVPacker()
{
	delete[] m_buf;
}

void FLVPacker::deliverVideoESPacket(const std::basic_string<std::uint8_t>& frame, unsigned int pts, bool iFrame)
{
	if (m_startVideoPts == 0)
		m_startVideoPts = pts;
	//offset&size
	auto dd = frame.c_str();
	std::vector<std::pair<size_t, size_t>> vo;
	GetNalus(frame, vo);

	if (vo.size() > 0)
	{
		if (m_sps.empty() && m_pps.empty())
		{
			if (vo.size() >= 3)//ÌáÈ¡sps pps
			{
				auto it = vo.begin();
				while (it != vo.end())
				{
					auto t = frame[it->first] & 0x1f;
					if (t == 7)
					{
						m_sps.append(frame.c_str() + it->first, it->second);
						it = vo.erase(it);
						continue;
					}
					else if (t == 8)
					{
						m_pps.append(frame.c_str() + it->first, it->second);
						it = vo.erase(it);
						continue;
					}
					++it;
				}
			}
		}

		if (!m_sps.empty() && !m_pps.empty())
		{
			auto temp = m_buf + 11;

			auto out1 = PackNalus(temp, frame, vo, iFrame);
			auto bodySize = out1 - temp;
			FLVTagHead(m_buf, FLV_TAG_TYPE::FLV_TAG_TYPE_VIDEO, bodySize, pts - m_startVideoPts);
			auto previousSize = bodySize + 11;

			*out1++ = (previousSize >> 24) & 0xff;
			*out1++ = (previousSize >> 16) & 0xff;
			*out1++ = (previousSize >> 8) & 0xff;
			*out1++ = previousSize & 0xff;
			if (m_maxBufferSize < bodySize)
			{
				int b = 2;
			}
			if (m_cb)
			{
				FLVFramePacket pk;
				pk.arg = this;
				pk.data.append(m_buf, bodySize + 15);
				pk.GetCodecInfo = std::bind(&FLVPacker::CodecInfo, this);
				m_cb(pk, m_arg);
			}
		}
	}
}

void FLVPacker::deliverAudioESPacket(const std::basic_string<std::uint8_t>& frame, unsigned int pts)
{
	return;
	if (m_startAudioPts == 0)
		m_startAudioPts = pts;

	auto temp = m_buf + 11;
	//a[0] = (10 << 4) | 0x0F;
	*temp++ = (10 << 4) | 0x0E;
	*temp++ = 0x01;
	memcpy(temp, frame.c_str() + 7, frame.size() - 7);
	temp += frame.size() - 7;
	auto bodySize = temp - m_buf - 11;
	FLVTagHead(m_buf, FLV_TAG_TYPE::FLV_TAG_TYPE_AUDIO, bodySize, pts - m_startAudioPts);

	size_t previousSize = bodySize + 11;
	*temp++ = (previousSize >> 24) & 0xff;
	*temp++ = (previousSize >> 16) & 0xff;
	*temp++ = (previousSize >> 8) & 0xff;
	*temp++ = previousSize & 0xff;
	if (m_cb)
	{
		FLVFramePacket f;
		f.type = "audio";
		f.data.append(m_buf, bodySize + 15);
		m_cb(f, m_arg);
	}
	return;
}

std::basic_string<std::uint8_t> FLVPacker::videoSequenceTag()
{
	std::basic_string<std::uint8_t> s;
	if (m_sps.empty() || m_pps.empty())
		return s;
	std::uint8_t a[512];
	auto temp = a + 11;

	*temp++ = 0x17;
	*temp++ = 0;//0 avcsequence
	*temp++ = 0;
	*temp++ = 0;
	*temp++ = 0;//compositionTime 
	*temp++ = 1;//version
	*temp++ = m_sps[1];
	*temp++ = m_sps[2];
	*temp++ = m_sps[3];
	*temp++ = 0xff;//6 bits reserved(111111) + 2 bits nal size length - 1 (11)
	*temp++ = 0xE1;//3 bits reserved (111) + 5 bits number of sps (00001)

	*temp++ = (m_sps.size() >> 8) & 0xff;
	*temp++ = m_sps.size() & 0xff;
	memcpy(temp, m_sps.c_str(), m_sps.size());
	temp += m_sps.size();

	*temp++ = 0x01;
	*temp++ = (m_pps.size() >> 8) & 0xff;
	*temp++ = m_pps.size() & 0xff;
	memcpy(temp, m_pps.c_str(), m_pps.size());
	temp += m_pps.size();

	auto bodySize = temp - a - 11;
	FLVTagHead(a, FLV_TAG_TYPE::FLV_TAG_TYPE_VIDEO, bodySize, 0);

	unsigned int previousSize = bodySize + 11;
	*temp++ = (previousSize >> 24) & 0xff;
	*temp++ = (previousSize >> 16) & 0xff;
	*temp++ = (previousSize >> 8) & 0xff;
	*temp++ = previousSize & 0xff;

	return std::basic_string<unsigned char>(a, bodySize + 15);
}

std::basic_string<std::uint8_t> FLVPacker::audioSequenceTag()
{
	std::basic_string<std::uint8_t> s;
	std::uint8_t a[19];
	auto temp = a + 11;
	//a[0] = (10 << 4) | 0x0F;//1010 1111 aac soundrate2 soundsize1 stereo1
	*temp++ = (10 << 4) | 0x0E;//1010 1111 aac soundrate2 soundsize1 stereo1
	*temp++ = 0x00;//sounddata
	*temp++ = 0x14;//00010 100  //16000kz 1soundtrack
	*temp++ = 0x08;//0 0001 0 0 0
	FLVTagHead(a, FLV_TAG_TYPE::FLV_TAG_TYPE_AUDIO, 4, 0);

	*temp++ = 0x00;
	*temp++ = 0x00;
	*temp++ = 0x00;
	*temp++ = 0x0f;

	return std::basic_string<unsigned char>(a, 19);
}

void FLVPacker::GetNalus(const std::basic_string<std::uint8_t>& data, std::vector<std::pair<size_t, size_t>>& vo)
{
	static std::uint8_t nalu[] = { 0x00,0x00,0x00,0x01 };
	size_t beg = 4;
	size_t end = 4;
	end = data.find(nalu, beg, 4);

	while (end != beg)
	{
		if (end == data.npos)
		{
			vo.push_back({ beg,data.size() - beg });
			break;
		}
		vo.push_back({ beg,end - beg });

		beg = end + 4;
		end = data.find(nalu, beg, 4);
	}
}

std::basic_string<std::uint8_t> FLVPacker::onMeta(double width, double heigth, double frameRate)
{
	unsigned char input[1024];
	auto temp = input + 11;
	AMF amf;
	temp = amf.AMF_EncodeString("onMetaData", temp);
	temp = amf.AMF_ArrayStart(13, temp);
	temp = amf.AMF_EncodeArrayItem("duration", 0., temp);
	temp = amf.AMF_EncodeArrayItem("width", 1920.0, temp);
	temp = amf.AMF_EncodeArrayItem("heigth", 1080.0, temp);
	temp = amf.AMF_EncodeArrayItem("videodatarate", 0., temp);
	temp = amf.AMF_EncodeArrayItem("framerate", 25.0, temp);
	temp = amf.AMF_EncodeArrayItem("videocodecid", 7.0, temp);
	temp = amf.AMF_EncodeArrayItem("audiodatarate", 0., temp);
	temp = amf.AMF_EncodeArrayItem("audiosamplerate", 16000.0, temp);
	temp = amf.AMF_EncodeArrayItem("audiosamplesize", 16.0, temp);
	temp = amf.AMF_EncodeArrayItem("stereo", false, temp);
	temp = amf.AMF_EncodeArrayItem("audiocodecid", 10.0, temp);
	temp = amf.AMF_EncodeArrayItem("encoder", "Lavf57.36.100", temp);
	temp = amf.AMF_EncodeArrayItem("filesize", 0.0, temp);
	temp = amf.AMF_EndObject(temp);
	auto bodySize = temp - input - 11;

	FLVTagHead(input, FLV_TAG_TYPE::FLV_TAG_TYPE_META, bodySize, 0);

	unsigned int previousSize = bodySize + 11;
	*temp++ = (previousSize >> 24) & 0xff;
	*temp++ = (previousSize >> 16) & 0xff;
	*temp++ = (previousSize >> 8) & 0xff;
	*temp++ = previousSize & 0xff;

	return std::basic_string<unsigned char>(input, bodySize + 15);
}

unsigned char* FLVPacker::FLVTagHead(unsigned char* in, FLV_TAG_TYPE t, size_t bodySize, size_t timeStamp)
{
	*in++ = static_cast<std::uint8_t>(t);
	//BodySize
	*in++ = (bodySize >> 16) & 0xff;
	*in++ = (bodySize >> 8) & 0xff;
	*in++ = bodySize & 0xff;
	//TimeStamp
	*in++ = (timeStamp >> 16) & 0xff;
	*in++ = (timeStamp >> 8) & 0xff;
	*in++ = timeStamp & 0xff;
	*in++ = (timeStamp >> 24) & 0xff;
	//StreamID
	*in++ = 0x00;
	*in++ = 0x00;
	*in++ = 0x00;

	return in;
}

unsigned char* FLVPacker::PackNalus(unsigned char* in, const std::basic_string<std::uint8_t>& data, std::vector<std::pair<size_t, size_t>>& vo, bool iFrame)
{
	if (iFrame)
		*in++ = 0x17;
	else
		*in++ = 0x27;
	*in++ = 0x01;//nalu
	*in++ = 0;
	*in++ = 0;
	*in++ = 0;

	auto it = vo.begin();
	while (it != vo.end())
	{
		*in++ = (it->second >> 24) & 0xff;
		*in++ = (it->second >> 16) & 0xff;
		*in++ = (it->second >> 8) & 0xff;
		*in++ = it->second & 0xff;
		memcpy(in, data.c_str() + it->first, it->second);
		in += it->second;

		++it;
	}
	return in;
}

std::basic_string<std::uint8_t> FLVPacker::CodecInfo()
{
	std::basic_string<std::uint8_t> data(FLV_FILE_HEADER, 13);
	data += m_meta;
	data += videoSequenceTag();
	data += audioSequenceTag();
	return data;
}