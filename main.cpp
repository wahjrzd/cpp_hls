// main.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "HLSServer.h"

#ifdef _DEBUG
#pragma comment(lib,"cpprest_2_10d.lib")
#else
#pragma comment(lib,"cpprest_2_10.lib")
#endif

#pragma comment(lib,"ws2_32.lib")

int main()
{
#if defined(DEBUG)|defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	WSADATA wd;
	WSAStartup(MAKEWORD(2, 2), &wd);

	WCHAR path[MAX_PATH];
	GetModuleFileName(NULL, path, MAX_PATH);
	wcsrchr(path, L'\\')[1] = 0;
	
	std::wstring cfgPath = path;
	cfgPath += L"cfg.ini";

	GetPrivateProfileString(L"Server", L"ip", L"127.0.0.1", path, MAX_PATH, cfgPath.c_str());
	
	std::wstring ip = path;
	
	unsigned short port = GetPrivateProfileInt(L"Server", L"port", 8123, cfgPath.c_str());
	HLSServer hls(ip, port);
	hls.Start();
	std::string line;
	std::getline(std::cin, line);
	hls.Stop();

	WSACleanup();
	return 0;
}