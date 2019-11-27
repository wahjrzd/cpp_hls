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

	HLSServer hls;
	hls.Start();
	std::string line;
	std::getline(std::cin, line);
	hls.Stop();

	WSACleanup();
	return 0;
}