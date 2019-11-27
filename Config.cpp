#include "Config.h"
#include <cpprest/json.h>

Config::Config() :_ip(U("127.0.0.1")), port(8123)
{
}

Config::~Config()
{
}

bool Config::LoadConfig(const std::string& cfgPath)
{
	try
	{
		web::json::value v;
		std::ifstream wf(cfgPath, std::wfstream::in);
		wf.seekg(0, std::ios::end);
		auto sz = std::streamoff(wf.tellg());
		wf.seekg(0, std::ios::beg);

		char* p = new char[sz];
		wf.read(p, sz);
		std::string out(p, sz);
		delete p;
	}
	catch (const std::exception& e)
	{
		std::clog << e.what() << std::endl;
		return false;
	}
	return true;
}