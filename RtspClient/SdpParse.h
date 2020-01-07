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
//fmtp
struct FMTPField
{
	int payload;
	std::map<std::string, std::string> kv;
};

class SdpParse
{
public:
	SdpParse();
	~SdpParse();

	int parse(const std::string& msg);
	bool GetMedia(const std::string& Type, Media& m);

	void ParseFmtp(FMTPField& fmtp, const std::string& src);
private:
	std::vector<std::string> SplitString(const std::string& src, char delim = ' ');
	void GetMediaInfo(std::stringstream& ss, std::string& line);
private:
	int version;
	std::string name;
	std::string info;
	std::string email;
	std::string uri;
	
	std::vector<Media> medias;
};

