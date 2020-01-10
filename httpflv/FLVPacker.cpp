#include "FLVPacker.h"
#include "AMF.h"

FLVPacker::FLVPacker() :m_cb(nullptr)
{
	m_meta = onMeta(1920, 1080, 25);
}

FLVPacker::~FLVPacker()
{

}

void FLVPacker::deliverVideoESPacket(const std::basic_string<std::uint8_t>& frame, unsigned int pts, bool iFrame)
{
	if (pppss == 0)
		pppss = pts;
	std::vector<std::basic_string<std::uint8_t>> vn;
	auto pp = frame.c_str();
	GetNalus(frame, vn);
	if (vn.size() > 0)
	{
		if (sps.empty() && pps.empty())
		{
			if (vn.size() >= 3)
			{
				sps = vn[0];
				pps = vn[1];

				if (m_cb)
				{
					FLVFramePacket f;
					f.VideoSeqFunc = std::bind(&FLVPacker::videoSequenceTag, this);
					f.AudioSeqFunc = std::bind(&FLVPacker::audioSequenceTag, this);
					f.MetaFunc = std::bind(&FLVPacker::metaTag, this);
					f.data = std::move(GenerateVideoFLVTag(vn, pts - pppss, iFrame));
					f.arg = this;
					m_cb(f, m_arg);
				}
				//pppss += 40;
			}
		}
		else
		{
			if (m_cb)
			{
				FLVFramePacket f;
				f.VideoSeqFunc = std::bind(&FLVPacker::videoSequenceTag, this);
				f.AudioSeqFunc = std::bind(&FLVPacker::audioSequenceTag, this);
				f.MetaFunc = std::bind(&FLVPacker::metaTag, this);
				f.data = std::move(GenerateVideoFLVTag(vn, pts - pppss, iFrame));
				f.arg = this;

				m_cb(f, m_arg);
			}
			//pppss += 40;
		}
	}
}

void FLVPacker::deliverAudioESPacket(const std::basic_string<std::uint8_t>& frame, unsigned int pts)
{
	return;
	if (lastPts == 0)
		lastPts = pts;
	std::basic_string<std::uint8_t> xx(frame.c_str() + 7, frame.size() - 7);
	auto data = GenerateAudioFLVTag(xx, pts - lastPts);
	if (m_cb)
	{
		FLVFramePacket f;
		f.VideoSeqFunc = std::bind(&FLVPacker::videoSequenceTag, this);
		f.AudioSeqFunc = std::bind(&FLVPacker::audioSequenceTag, this);
		f.MetaFunc = std::bind(&FLVPacker::metaTag, this);
		f.arg = this;
		f.data = std::move(data);
		f.type = "audio";
		m_cb(f, m_arg);
	}
}

std::basic_string<std::uint8_t> FLVPacker::GenerateFLVTagHeader(FLV_TAG_TYPE t, unsigned int bodySize, unsigned int timeStamp)
{
	std::basic_string<std::uint8_t> s;
	std::uint8_t a[11];
	a[0] = static_cast<std::uint8_t>(t);
	a[1] = (bodySize >> 16) & 0xff;
	a[2] = (bodySize >> 8) & 0xff;
	a[3] = bodySize & 0xff;

	a[4] = (timeStamp >> 16) & 0xff;
	a[5] = (timeStamp >> 8) & 0xff;
	a[6] = timeStamp & 0xff;
	a[7] = (timeStamp >> 24) & 0xff;
	a[8] = 0;
	a[9] = 0;
	a[10] = 0;
	s.append(a, 11);
	return s;
}

std::basic_string<std::uint8_t> FLVPacker::videoSequenceTag()
{
	std::basic_string<std::uint8_t> s;
	if (sps.empty() || pps.empty())
		return s;

	std::uint8_t a[11];
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
	s.append(a, 11);

	a[0] = (sps.size() >> 8) & 0xff;
	a[1] = sps.size() & 0xff;
	s.append(a, 2);
	s.append(sps);

	a[0] = 0x01;
	a[1] = (pps.size() >> 8) & 0xff;
	a[2] = pps.size() & 0xff;
	s.append(a, 3);
	s.append(pps);

	auto tagHead = GenerateFLVTagHeader(FLV_TAG_TYPE::FLV_TAG_TYPE_VIDEO, s.size(), 0);

	unsigned int previousSize = s.size() + 11;
	a[0] = (previousSize >> 24) & 0xff;
	a[1] = (previousSize >> 16) & 0xff;
	a[2] = (previousSize >> 8) & 0xff;
	a[3] = previousSize & 0xff;

	std::basic_string<std::uint8_t> ret(std::move(tagHead));
	ret.append(s);
	ret.append(a, 4);
	return ret;
}

std::basic_string<std::uint8_t> FLVPacker::GenerateVideoFLVTag(std::vector<std::basic_string<std::uint8_t>>& nalus,
	unsigned int timeStamp, bool iFrame)
{
	std::basic_string<std::uint8_t> s;
	auto videoData = PackNalus(nalus, iFrame);
	auto tagHead = GenerateFLVTagHeader(FLV_TAG_TYPE::FLV_TAG_TYPE_VIDEO, videoData.size(), timeStamp);

	unsigned int previousSize = videoData.size() + 11;
	std::uint8_t a[4];
	a[0] = (previousSize >> 24) & 0xff;
	a[1] = (previousSize >> 16) & 0xff;
	a[2] = (previousSize >> 8) & 0xff;
	a[3] = previousSize & 0xff;

	s.append(std::move(tagHead));
	s.append(std::move(videoData));
	s.append(a, 4);

	return s;
}

std::basic_string<std::uint8_t> FLVPacker::audioSequenceTag()
{
	std::basic_string<std::uint8_t> s;
	std::uint8_t a[4];
	//a[0] = (10 << 4) | 0x0F;//1010 1111 aac soundrate2 soundsize1 stereo1
	a[0] = (10 << 4) | 0x0E;//1010 1111 aac soundrate2 soundsize1 stereo1
	a[1] = 0x00;//sounddata
	a[2] = 0x14;//00010 100  //16000kz 1soundtrack
	a[3] = 0x08;//0 0001 0 0 0
	auto tagHead = GenerateFLVTagHeader(FLV_TAG_TYPE::FLV_TAG_TYPE_AUDIO, 4, 0);

	s.append(tagHead);
	s.append(a, 4);

	unsigned int previousSize = 4 + 11;
	a[0] = (previousSize >> 24) & 0xff;
	a[1] = (previousSize >> 16) & 0xff;
	a[2] = (previousSize >> 8) & 0xff;
	a[3] = previousSize & 0xff;
	
	s.append(a, 4);
	return s;
}

std::basic_string<std::uint8_t> FLVPacker::GenerateAudioFLVTag(const std::basic_string<std::uint8_t>& data, unsigned int timeStamp)
{
	std::basic_string<std::uint8_t> s;
	std::uint8_t a[4];
	//a[0] = (10 << 4) | 0x0F;
	a[0] = (10 << 4) | 0x0E;
	a[1] = 1;
	s.append(a, 2);
	s.append(data);

	auto tagHead = GenerateFLVTagHeader(FLV_TAG_TYPE::FLV_TAG_TYPE_AUDIO, s.size(), timeStamp);
	unsigned int previousSize = s.size() + 11;
	a[0] = (previousSize >> 24) & 0xff;
	a[1] = (previousSize >> 16) & 0xff;
	a[2] = (previousSize >> 8) & 0xff;
	a[3] = previousSize & 0xff;

	std::basic_string<std::uint8_t> ret(std::move(tagHead));
	ret.append(s);
	ret.append(a, 4);
	return ret;
}

std::basic_string<std::uint8_t> FLVPacker::PackNalus(std::vector<std::basic_string<std::uint8_t>>& vn, bool iFrame)
{
	std::basic_string<std::uint8_t> s;
	std::uint8_t a[5];
	
	if (iFrame)
		a[0] = 0x17;
	else
		a[0] = 0x27;
	a[1] = 0x01;//nalu
	a[2] = 0;
	a[3] = 0;
	a[4] = 0;
	s.append(a, 5);
	auto it = vn.begin();
	while (it != vn.end())
	{
		a[0] = (it->size() >> 24) & 0xff;
		a[1] = (it->size() >> 16) & 0xff;
		a[2] = (it->size() >> 8) & 0xff;
		a[3] = it->size() & 0xff;
		s.append(a, 4);
		s.append(*it);
		++it;
	}
	
	return s;
}

void FLVPacker::GetNalus(const std::basic_string<std::uint8_t>& data, std::vector<std::basic_string<std::uint8_t>>& vn)
{
	static std::uint8_t nalu[] = { 0x00,0x00,0x00,0x01 };
	size_t beg = 4;
	size_t end = 4;
	end = data.find(nalu, beg, 4);
	
	while (end != beg)
	{
		auto item = data.substr(beg, end - beg);
		vn.push_back(std::move(item));
		if (end == data.npos)
			break;

		beg = end + 4;
		end = data.find(nalu, beg, 4);
	}
}

std::basic_string<std::uint8_t> FLVPacker::onMeta(double width, double heigth, double frameRate)
{ 
	unsigned char input[1024];
	auto temp = input;
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
	auto xxxx = temp - input;

	auto tagHead = GenerateFLVTagHeader(FLV_TAG_TYPE::FLV_TAG_TYPE_META, xxxx, 0);
	unsigned int previousSize = xxxx + 11;
	*temp++ = (previousSize >> 24) & 0xff;
	*temp++ = (previousSize >> 16) & 0xff;
	*temp++ = (previousSize >> 8) & 0xff;
	*temp++ = previousSize & 0xff;

	tagHead.append(input, xxxx + 4);
	return tagHead;
}