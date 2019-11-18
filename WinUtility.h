#pragma once
#include <string>
#include <Windows.h>
#include <vector>

#define MD5LEN  16

class WinUtility
{
public:
	WinUtility();
	~WinUtility();

	static std::string Base64Encode(const BYTE* data, DWORD sz);
	static std::basic_string<BYTE> Base64Decode(const char* data, DWORD sz);
	static std::string UnicodeToAnsi(const WCHAR* data, DWORD sz);
	static std::wstring AnisToUnicode(const char* data, DWORD sz);
	static std::string MD5Encode(const BYTE* data, DWORD sz);
	static std::string CreateXID();
private:
	static BYTE encoding[32];
};

