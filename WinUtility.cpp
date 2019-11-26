#include "pch.h"
#include "WinUtility.h"
#include <wincrypt.h>
#include <random>

#pragma comment(lib,"Crypt32.lib")
BYTE WinUtility::encoding[] = {
	'0' ,'1','2','3','4','5','6','7','8','9',
	'a' ,'b','c','d','e','f','g','h','i','j',
	'k' ,'l','m','n','o','p','q','r','s','t',
	'u' ,'v'
};

WinUtility::WinUtility()
{
}


WinUtility::~WinUtility()
{
}

std::string WinUtility::Base64Encode(const BYTE* data, DWORD sz)
{
	std::string retStr;

	DWORD dwRet = 24;
	if (CryptBinaryToString(data, sz, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &dwRet))
	{
		WCHAR* p = new WCHAR[dwRet];
		CryptBinaryToString(data, sz, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, p, &dwRet);
		retStr = UnicodeToAnsi(p, dwRet);
		delete p;
	}
	return retStr;
}

std::basic_string<BYTE> WinUtility::Base64Decode(const char* data, DWORD sz)
{
	auto str = AnisToUnicode(data, sz);

	std::basic_string<BYTE> bs;
	DWORD dwRet;
	if (CryptStringToBinary(str.c_str(), str.size(), CRYPT_STRING_BASE64, nullptr, &dwRet, nullptr, nullptr))
	{
		BYTE* p = new BYTE[dwRet];
		CryptStringToBinary(str.c_str(), str.size(), CRYPT_STRING_BASE64, p, &dwRet, nullptr, nullptr);
		bs.append(p, dwRet);
		delete p;
	}
	return bs;
}

std::string WinUtility::UnicodeToAnsi(const WCHAR* data, DWORD sz)
{
	int ret;
	ret = WideCharToMultiByte(CP_ACP, 0, data, sz, nullptr, 0, nullptr, FALSE);
	if (ret == -1)
	{
		fprintf(stderr, "WideCharToMultiByte failed:%u\n", GetLastError());
		return "";
	}
	char* ac = new char[ret];
	WideCharToMultiByte(CP_ACP, 0, data, sz, ac, ret, nullptr, FALSE);
	std::string retStr(ac, ret);
	delete ac;
	return retStr;
}

std::wstring WinUtility::AnisToUnicode(const char* data, DWORD sz)
{
	int ret;
	ret = MultiByteToWideChar(CP_ACP, 0, data, sz, nullptr, 0);
	if (ret == -1)
	{
		fprintf(stderr, "MultiByteToWideChar failed:%u\n", GetLastError());
		return L"";
	}
	WCHAR* wc = new WCHAR[ret];
	MultiByteToWideChar(CP_ACP, 0, data, sz, wc, ret);
	std::wstring retStr(wc, ret);
	delete wc;
	return retStr;
}

std::string WinUtility::MD5Encode(const BYTE* data, DWORD sz)
{
	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;

	if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
	{
		fprintf(stderr, "CryptAcquireContext failed:%u\n", GetLastError());
		return "";
	}
	if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
	{
		fprintf(stderr, "CryptCreateHash failed:%u\n", GetLastError());
		CryptReleaseContext(hProv, 0);
		return "";
	}

	if (!CryptHashData(hHash, data, sz, 0))
	{
		fprintf(stderr, "CryptHashData failed:%u\n", GetLastError());
		CryptDestroyHash(hHash);
		CryptReleaseContext(hProv, 0);
		return "";
	}

	DWORD cbHash = MD5LEN;
	BYTE rgbHash[MD5LEN];
	CHAR rgbDigits[] = "0123456789abcdef";
	std::string retStr;
	if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
	{
		for (DWORD i = 0; i < cbHash; i++)
		{
			retStr.push_back(rgbDigits[rgbHash[i] >> 4]);
			retStr.push_back(rgbDigits[rgbHash[i] & 0xf]);
		}
	}

	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);
	return retStr;
}

std::string WinUtility::CreateXID()
{
	BYTE id[12];
	DWORD pid = GetCurrentProcessId();
	HW_PROFILE_INFO hi;
	GetCurrentHwProfile(&hi);

	std::string machineID = MD5Encode((BYTE*)hi.szHwProfileGuid, 38 * 2);

	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);

	id[0] = (now.QuadPart >> 24) & 0xff;
	id[1] = (now.QuadPart >> 16) & 0xff;
	id[2] = (now.QuadPart >> 8) & 0xff;
	id[3] = now.QuadPart & 0xff;

	id[4] = machineID[0];
	id[5] = machineID[1];
	id[6] = machineID[3];

	id[7] = (pid >> 8) & 0xff;
	id[8] = pid & 0xff;

	std::default_random_engine e;
	e.seed(static_cast<unsigned int>(now.QuadPart));

	auto i = e();
	id[9] = (i >> 16) & 0xff;
	id[10] = (i >> 8) & 0xff;
	id[11] = i & 0xff;

	BYTE dst[20];
	dst[19] = encoding[(id[11] << 4) & 0x1F];
	dst[18] = encoding[(id[11] >> 1) & 0x1F];
	dst[17] = encoding[(id[11] >> 6) & 0x1F | (id[10] << 2) & 0x1F];
	dst[16] = encoding[id[10] >> 3];
	dst[15] = encoding[id[9] & 0x1F];
	dst[14] = encoding[(id[9] >> 5) | (id[8] << 3) & 0x1F];
	dst[13] = encoding[(id[8] >> 2) & 0x1F];
	dst[12] = encoding[id[8] >> 7 | (id[7] << 1) & 0x1F];
	dst[11] = encoding[(id[7] >> 4) & 0x1F | (id[6] << 4) & 0x1F];
	dst[10] = encoding[(id[6] >> 1) & 0x1F];
	dst[9] = encoding[(id[6] >> 6) & 0x1F | (id[5] << 2) & 0x1F];
	dst[8] = encoding[id[5] >> 3];
	dst[7] = encoding[id[4] & 0x1F];
	dst[6] = encoding[id[4] >> 5 | (id[3] << 3) & 0x1F];
	dst[5] = encoding[(id[3] >> 2) & 0x1F];
	dst[4] = encoding[id[3] >> 7 | (id[2] << 1) & 0x1F];
	dst[3] = encoding[(id[2] >> 4) & 0x1F | (id[1] << 4) & 0x1F];
	dst[2] = encoding[(id[1] >> 1) & 0x1F];
	dst[1] = encoding[(id[1] >> 6) & 0x1F | (id[0] << 2) & 0x1F];
	dst[0] = encoding[id[0] >> 3];
	return std::string((char*)dst, 20);
}