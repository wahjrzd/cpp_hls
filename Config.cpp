#include "Config.h"
#include <cpprest/json.h>

Config::Config() :_ip(U("127.0.0.1")), _port(8123)
{
}

Config::~Config()
{
}

bool Config::LoadConfig(const std::wstring& cfgPath)
{
	try
	{
		utility::ifstream_t wf(cfgPath, std::wfstream::in | std::wfstream::binary);
		auto retJson = web::json::value::parse(wf);
		_ip = retJson.at(U("host")).as_string();
		_port = retJson.at(U("port")).as_integer();
		auto stream = retJson.at(U("stream"));
		if (stream.is_array())
		{
			auto it = stream.as_array().cbegin();
			while (it != stream.as_array().cend())
			{
				auto id = utility::conversions::to_utf8string(it->at(U("id")).as_string());
				auto url = utility::conversions::to_utf8string(it->at(U("url")).as_string());
				streamMap.insert({ std::move(id) ,std::move(url) });
				++it;
			}
		}
	}
	catch (const std::exception& e)
	{
		std::clog << e.what() << std::endl;
		return false;
	}
	return true;
}