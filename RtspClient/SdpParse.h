#pragma once
#include <string>
#include <vector>
#include <map>

struct Media
{
	std::string Type;
	unsigned short port;
	std::string Protocol;
	std::vector<std::string> Formats;
	std::map<std::string, std::string> Attributes;
	Media()
	{
		port = 0;
	}
};

class SdpParse
{
public:
	SdpParse();
	~SdpParse();

	int parse(const std::string& msg);
	bool GetMedia(const std::string& Type, Media& m);
private:
	std::vector<std::string> SplitString(const std::string& src, char delim = ' ');
	void GetMedia(std::stringstream& ss, std::string& line);
private:
	int version;
	std::string name;
	std::string info;
	std::string email;
	std::string uri;
	
	std::vector<Media> medias;
};

