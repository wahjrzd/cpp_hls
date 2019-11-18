// SimpleHls.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "HLSServer.h"

#pragma comment(lib,"Httpapi.lib")
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
	unsigned short port = GetProfileInt(L"Server", L"port", 8123);
	HLSServer hls(ip, port);
	hls.Start();


	WSACleanup();
	return 0;
}