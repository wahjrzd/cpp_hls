#pragma once

#include <string>
#include <map>

class HLSServer;

class Config
{
public:
	Config();
	~Config();

	bool LoadConfig(const std::wstring& cfgPath);
private:
	std::wstring _ip;
	unsigned short _port;
	std::map<std::string, std::string> streamMap;

	friend HLSServer;
};

