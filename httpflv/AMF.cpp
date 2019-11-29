#include "AMF.h"

AMF::AMF()
{
}

AMF::~AMF()
{
}

unsigned char* AMF::AMF_EncodeBoolean(unsigned char* input, bool val)
{
	*input++ = static_cast<unsigned char>(AMFDataType::AMF_BOOLEAN);
	*input++ = val == true ? 0x01 : 0x00;
	return input;
}

unsigned char* AMF::AMF_Encodestr(unsigned char* input, unsigned char* str, unsigned int sz)
{
	*input++ = static_cast<unsigned char>(AMFDataType::AMF_STRING);
	*input++ = (sz >> 8) & 0xff;
	*input++ = sz & 0xff;
	memcpy(input, str, sz);
	input += sz;
	return input;
}

unsigned char* AMF::AMF_Encodenum(unsigned char* input, double val)
{
	*input++ = static_cast<unsigned char>(AMFDataType::AMF_NUMBER);
	unsigned char* ci = (unsigned char*)&val;
	*input++ = ci[7];
	*input++ = ci[6];
	*input++ = ci[5];
	*input++ = ci[4];
	*input++ = ci[3];
	*input++ = ci[2];
	*input++ = ci[1];
	*input++ = ci[0];
	return input;
}

std::string AMF::AMF_EncodeString(const std::string& name)
{
	std::string s;
	char a[3];
	a[0] = static_cast<char>(AMFDataType::AMF_STRING);
	a[1] = (name.size() >> 8) & 0xff;
	a[2] = name.size() & 0xff;
	s.append(a, 3);
	s.append(name);
	return s;
}

std::string AMF::AMF_EncodeNumber(const double val)
{
	std::string s;
	char* ci = (char*)&val;
	char a[9];
	a[0] = static_cast<char>(AMFDataType::AMF_NUMBER);
	a[1] = ci[7];
	a[2] = ci[6];
	a[3] = ci[5];
	a[4] = ci[4];
	a[5] = ci[3];
	a[6] = ci[2];
	a[7] = ci[1];
	a[8] = ci[0];
	s.append(a, 9);
	return s;
}

std::string AMF::AMF_EncodePropertyWithString(const std::string& propertyName, const std::string& val)
{
	std::string s;
	char a[2];
	a[0] = (propertyName.size() >> 8) & 0xff;
	a[1] = propertyName.size() & 0xff;
	s.append(a, 2);
	s.append(propertyName);
	s.append(AMF_EncodeString(val));
	return s;
}

std::string AMF::AMF_EncodePropertyWithDouble(const std::string& propertyName, double val)
{
	std::string s;
	char a[2];
	a[0] = (propertyName.size() >> 8) & 0xff;
	a[1] = propertyName.size() & 0xff;
	s.append(a, 2);
	s.append(propertyName);
	s.append(AMF_EncodeNumber(val));
	return s;
}