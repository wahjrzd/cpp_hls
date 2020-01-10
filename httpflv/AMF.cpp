#include "AMF.h"

AMF::AMF()
{
	m_buf = new unsigned char[32];
}

AMF::~AMF()
{
	delete m_buf;
}

unsigned char* AMF::AMF_EncodeBoolean(bool val, unsigned char* input)
{
	*input++ = static_cast<unsigned char>(AMFDataType::AMF_BOOLEAN);
	*input++ = val == true ? 0x01 : 0x00;
	return input;
}

unsigned char* AMF::AMF_EncodeString(const char* value, unsigned char* input)
{
	auto strLen = strlen(value);
	*input++ = static_cast<unsigned char>(AMFDataType::AMF_STRING);
	input = AMF_EncodeInt16(strLen, input);
	memcpy(input, value, strLen);
	input += strLen;
	return input;
}

unsigned char* AMF::AMF_EncodeInt16(int16_t value, unsigned char* input)
{
	*input++ = value >> 8;
	*input++ = value;
	return input;
}

unsigned char* AMF::AMF_EncodeInt32(int32_t value, unsigned char* input)
{
	*input++ = value >> 24;
	*input++ = value >> 16;
	*input++ = value >> 8;
	*input++ = value;
	return input;
}

unsigned char* AMF::AMF_EncodeNumber(double value, unsigned char* input)
{
	*input++ = static_cast<unsigned char>(AMFDataType::AMF_NUMBER);
	unsigned char* dv = (unsigned char*)&value;
	*input++ = dv[7];
	*input++ = dv[6];
	*input++ = dv[5];
	*input++ = dv[4];
	*input++ = dv[3];
	*input++ = dv[2];
	*input++ = dv[1];
	*input++ = dv[0];

	return input;
}

unsigned char* AMF::AMF_ArrayStart(int items, unsigned char* input)
{
	*input++ = static_cast<unsigned char>(AMFDataType::AMF_ECMA_ARRAY);
	input = AMF_EncodeInt32(items, input);
	return input;
}

unsigned char* AMF::AMF_EncodeArrayItem(const char* name, double value, unsigned char* input)
{
	auto strLen = strlen(name);
	input = AMF_EncodeInt16(strLen, input);
	memcpy(input, name, strLen);
	input += strLen;

	input = AMF_EncodeNumber(value, input);
	return input;
}

unsigned char* AMF::AMF_EncodeArrayItem(const char* name, bool value, unsigned char* input)
{
	auto strLen = strlen(name);
	input = AMF_EncodeInt16(strLen, input);
	memcpy(input, name, strLen);
	input += strLen;

	input = AMF_EncodeBoolean(value, input);
	return input;
}

unsigned char* AMF::AMF_EncodeArrayItem(const char* name, const char* value, unsigned char* input)
{
	auto strLen = strlen(name);
	input = AMF_EncodeInt16(strLen, input);
	memcpy(input, name, strLen);
	input += strLen;

	input = AMF_EncodeString(value, input);
	return input;
}

unsigned char* AMF::AMF_EndObject(unsigned char* input)
{
	*input++ = 0;
	*input++ = 0;
	*input++ = static_cast<unsigned char>(AMFDataType::AMF_OBJECT_END);
	return input;
}