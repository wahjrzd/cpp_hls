#pragma once

#include <string>
#include <map>

class HLSServer;

class Config
{
public:
	Config();
	~Config();

	bool LoadConfig(const std::string& cfgPath);
private:
	std::wstring _ip;
	unsigned short port;
	std::map<std::string, std::string> streamMap;

	friend HLSServer;
};

