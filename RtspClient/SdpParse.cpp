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
			GetMedia(ss, line);
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
	size_t i = 0;
	auto pos = src.find(delim, i);
	while (pos != src.npos)
	{
		auto item = src.substr(i, pos - i);
		if (!item.empty())
			v.push_back(item);
		i = pos + 1;

		pos = src.find(delim, i);
		if (pos == src.npos && i < src.size())
			pos = src.size();
	}
	return v;
}

void SdpParse::GetMedia(std::stringstream& ss, std::string& line)
{
	Media m;
	auto infos = SplitString(std::string(line.c_str() + 2));
	if (infos.size() >= 4)
	{
		auto it = infos.begin();
		m.Type = *it;
		++it;
		m.port = std::stoi(*it);
		++it;
		m.Protocol = *it;
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
			GetMedia(ss, line);
		}
		else if (line.empty())
			break;
	}
	medias.push_back(m);
}