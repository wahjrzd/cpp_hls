#include "FLVPacker.h"

FLVPacker::FLVPacker() :m_cb(nullptr)
{

}

FLVPacker::~FLVPacker()
{

}

void FLVPacker::deliverVideoESPacket(const std::basic_string<std::uint8_t>& frame, unsigned int pts, bool iFrame)
{
	std::vector<std::basic_string<std::uint8_t>> vn;
	auto pp = frame.c_str();
	GetNalus(frame, vn);
	if (vn.size() > 0)
	{
		if (m_videoSequenceHeader.empty())
		{
			if (vn.size() > 3)
			{
				GenerateAVCSequenceHeader(vn[0], vn[1]);
			}
		}
		else
		{
			auto videoTag = PackOneFrame(vn, iFrame);
			auto tagHead = GenerateFLVTagHeader(FLV_TAG_TYPE::FLV_TAG_TYPE_VIDEO, videoTag.size(), pts);
			m_tagData.append(std::move(tagHead));
			m_tagData.append(std::move(videoTag));
			size_t previousSize = videoTag.size() + 11;
			std::uint8_t a[4];
			a[0] = (previousSize >> 24) & 0xff;
			a[1] = (previousSize >> 16) & 0xff;
			a[2] = (previousSize >> 8) & 0xff;
			a[3] = previousSize & 0xff;
			m_tagData.append(a, 4);
			if (m_cb)
			{
				FLVFramePacket f;
				f.data.append(m_tagData);
				m_cb(f, m_arg);
			}
			m_tagData.clear();
		}
	}
}

void FLVPacker::deliverAudioESPacket(const std::basic_string<std::uint8_t>& frame, unsigned int pts)
{

}

std::basic_string<std::uint8_t> FLVPacker::GenerateFLVTagHeader(FLV_TAG_TYPE t, unsigned int bodySize, unsigned int timeStamp)
{
	std::basic_string<std::uint8_t> s;
	unsigned char a[11];
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

void FLVPacker::GenerateAVCSequenceHeader(const std::basic_string<std::uint8_t>& sps, const std::basic_string<std::uint8_t>& pps)
{
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
	m_videoSequenceHeader.append(a, 10);

	a[0] = (sps.size() >> 8) & 0xff;
	a[1] = sps.size() & 0xff;
	m_videoSequenceHeader.append(a, 2);
	m_videoSequenceHeader.append(sps);

	a[0] = 0x01;
	a[1] = (pps.size() >> 8) & 0xff;
	a[2] = pps.size() & 0xff;
	m_videoSequenceHeader.append(a, 3);
	m_videoSequenceHeader.append(pps);
}

std::basic_string<std::uint8_t> FLVPacker::GenerateAudioTagHead()
{
	std::basic_string<std::uint8_t> s;
	return s;
}

std::basic_string<std::uint8_t> FLVPacker::PackOneFrame(std::vector<std::basic_string<std::uint8_t>>& vn, bool iFrame)
{
	std::basic_string<std::uint8_t> s;
	unsigned char a[9];
	
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
	static unsigned char nalu[] = { 0x00,0x00,0x00,0x01 };
	size_t beg = 4;
	size_t end = 4;
	end = data.find_first_of(nalu, beg, 4);
	
	while (end != beg)
	{
		if (end == data.npos)
		{
			auto item = data.substr(beg, data.size());
			vn.push_back(std::move(item));
			break;
		}
		else
		{
			auto item = data.substr(beg, end - beg);
			auto sz = item.size();
			auto yy = item.c_str();
			vn.push_back(std::move(item));
		}

		beg = end + 4;
		end = data.find_first_of(nalu, beg + 1, 4);
		int xx = 0;
	}
}