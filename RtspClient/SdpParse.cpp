#include "SdpParse.h"
#include <sstream>

SdpParse::SdpParse()
{
}


SdpParse::~SdpParse()
{	
}

int SdpParse::parse(const std::string& msg)
{
	std::stringstream ss(msg);
	std::string line;
	
	while (true)
	{
		auto& stream = std::getline(ss, line);
		
		if (stream.eof())
			break;
		line.pop_back();
		if (strncmp(line.c_str(), "m=", 2) == 0)
		{
			GetMediaInfo(ss, line);
		}
	}
	
	return 0;
}

bool SdpParse::GetMedia(const std::string& Type, Media& m)
{
	auto it = medias.begin();
	while (it != medias.end())
	{
		if (it->Type == Type)
		{
			m = *it;
			return true;
		}
		++it;
	}
	return false;
}

std::vector<std::string> SdpParse::SplitString(const std::string& src, char delim)
{
	std::vector<std::string> v;
	std::stringstream ss(src);
	std::string item;
	while (std::getline(ss, item, delim))
	{
		v.push_back(item);
	}

	return v;
}

void SdpParse::GetMediaInfo(std::stringstream& ss, std::string& line)
{
	Media m;
	auto infos = SplitString(std::string(line.c_str() + 2));
	if (infos.size() >= 4)
	{
		auto it = infos.begin();
		m.Type = *it;//video
		++it;
		m.port = std::stoi(*it);
		++it;
		m.Protocol = *it;//RTP/AVP
		++it;
		while (it != infos.end())
		{
			m.Formats.push_back(*it);
			++it;
		}
	}

	while (true)
	{
		auto& stream = std::getline(ss, line);
		if (stream.eof())
			break;
		line.pop_back();
		if (strncmp(line.c_str(), "a=", 2) == 0)
		{
			auto attribute = line.substr(2);
			auto pos = attribute.find(':');
			if (pos != attribute.npos)
			{
				auto key = attribute.substr(0, pos);
				auto value = attribute.substr(pos + 1);

				m.Attributes.insert({ key,value });
			}
		}
		else if (strncmp(line.c_str(), "m=", 2) == 0)
		{
			GetMediaInfo(ss, line);
		}
		else if (line.empty())
			break;
	}
	medias.push_back(m);
}

void SdpParse::ParseFmtp(FMTPField& fmtp, const std::string& src)
{
	auto pos = src.find(' ');
	if (pos < 5 && pos != src.npos)
	{
		fmtp.payload = std::stoi(src.substr(0, pos));
		std::stringstream ss(src.substr(pos + 1));

		std::string item;
		while (std::getline(ss, item, ';'))
		{
			if (item[0] == ' ')
				item.erase(item.begin());
			auto pos = item.find('=');
			if (pos != item.npos)
			{
				item.substr(pos);
				fmtp.kv.insert({ item.substr(0,pos) ,item.substr(pos + 1) });
			}
		}
	}

	
}