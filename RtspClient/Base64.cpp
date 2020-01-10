#include "Base64.h"
#include <mutex>

std::once_flag Base64::m_flag;

Base64::Base64()
{
	std::call_once(m_flag, &Base64::initBase64DecodeTable, this);
}

Base64::~Base64()
{
}

void Base64::initBase64DecodeTable()
{
	int i;
	for (i = 0; i < 256; ++i) base64DecodeTable[i] = (char)0x80;
	// default value: invalid

	for (i = 'A'; i <= 'Z'; ++i) base64DecodeTable[i] = 0 + (i - 'A');
	for (i = 'a'; i <= 'z'; ++i) base64DecodeTable[i] = 26 + (i - 'a');
	for (i = '0'; i <= '9'; ++i) base64DecodeTable[i] = 52 + (i - '0');
	base64DecodeTable[(unsigned char)'+'] = 62;
	base64DecodeTable[(unsigned char)'/'] = 63;
	base64DecodeTable[(unsigned char)'='] = 0;
}

unsigned char* Base64::base64Decode(char const* in, unsigned inSize,
	unsigned& resultSize, bool trimTrailingZeros)
{
	unsigned char* out = new unsigned char[inSize + 1]; // ensures we have enough space
	int k = 0;
	int paddingCount = 0;
	int const jMax = inSize - 3;
	// in case "inSize" is not a multiple of 4 (although it should be)
	for (int j = 0; j < jMax; j += 4)
	{
		char inTmp[4], outTmp[4];
		for (int i = 0; i < 4; ++i)
		{
			inTmp[i] = in[i + j];
			if (inTmp[i] == '=') ++paddingCount;
			outTmp[i] = base64DecodeTable[(unsigned char)inTmp[i]];
			if ((outTmp[i] & 0x80) != 0) outTmp[i] = 0; // this happens only if there was an invalid character; pretend that it was 'A'
		}

		out[k++] = (outTmp[0] << 2) | (outTmp[1] >> 4);
		out[k++] = (outTmp[1] << 4) | (outTmp[2] >> 2);
		out[k++] = (outTmp[2] << 6) | outTmp[3];
	}

	if (trimTrailingZeros)
	{
		while (paddingCount > 0 && k > 0 && out[k - 1] == '\0') { --k; --paddingCount; }
	}
	resultSize = k;
	unsigned char* result = new unsigned char[resultSize];
	memmove(result, out, resultSize);
	delete[] out;

	return result;
}

const char base64Char[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char* Base64::base64Encode(char const* origSigned, unsigned origLength)
{
	unsigned char const* orig = (unsigned char const*)origSigned; // in case any input bytes have the MSB set
	if (orig == NULL) return NULL;

	unsigned const numOrig24BitValues = origLength / 3;
	bool havePadding = origLength > numOrig24BitValues * 3;
	bool havePadding2 = origLength == numOrig24BitValues * 3 + 2;
	unsigned const numResultBytes = 4 * (numOrig24BitValues + havePadding);
	char* result = new char[numResultBytes + 1]; // allow for trailing '\0'

	// Map each full group of 3 input bytes into 4 output base-64 characters:
	unsigned i;
	for (i = 0; i < numOrig24BitValues; ++i) {
		result[4 * i + 0] = base64Char[(orig[3 * i] >> 2) & 0x3F];
		result[4 * i + 1] = base64Char[(((orig[3 * i] & 0x3) << 4) | (orig[3 * i + 1] >> 4)) & 0x3F];
		result[4 * i + 2] = base64Char[((orig[3 * i + 1] << 2) | (orig[3 * i + 2] >> 6)) & 0x3F];
		result[4 * i + 3] = base64Char[orig[3 * i + 2] & 0x3F];
	}

	// Now, take padding into account.  (Note: i == numOrig24BitValues)
	if (havePadding) {
		result[4 * i + 0] = base64Char[(orig[3 * i] >> 2) & 0x3F];
		if (havePadding2) {
			result[4 * i + 1] = base64Char[(((orig[3 * i] & 0x3) << 4) | (orig[3 * i + 1] >> 4)) & 0x3F];
			result[4 * i + 2] = base64Char[(orig[3 * i + 1] << 2) & 0x3F];
		}
		else {
			result[4 * i + 1] = base64Char[((orig[3 * i] & 0x3) << 4) & 0x3F];
			result[4 * i + 2] = '=';
		}
		result[4 * i + 3] = '=';
	}

	result[numResultBytes] = '\0';
	return result;
}