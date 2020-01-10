#pragma once
#include <string>

class Base64
{
public:
	Base64();
	~Base64();

	// returns a newly allocated array - of size "resultSize" - that
	// the caller is responsible for delete[]ing.
	unsigned char* base64Decode(char const* in, unsigned inSize,
		unsigned& resultSize, bool trimTrailingZeros = true);

	// returns a 0-terminated string that
   // the caller is responsible for delete[]ing.
	char* base64Encode(char const* orig, unsigned origLength);
private:
	void initBase64DecodeTable();
private:
	static std::once_flag m_flag;
private:
	char base64DecodeTable[256];
};