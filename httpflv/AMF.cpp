#include "AMF.h"

AMF::AMF()
{
}

AMF::~AMF()
{
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

std::string AMF::AMF_EncodePropertyWithDouble(const std::string& propertyName, const double& val)
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