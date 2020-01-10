#pragma once
#include <string>

enum class AMFDataType
{
	AMF_NUMBER,
	AMF_BOOLEAN,
	AMF_STRING,
	AMF_OBJECT,
	AMF_MOVIECLIP,
	AMF_NULL,
	AMF_UNDEFINED,
	AMF_REFERENCE,
	AMF_ECMA_ARRAY,
	AMF_OBJECT_END,
	AMF_STRICT_ARRAY,
	AMF_DATE,
	AMF_LONG_STRING,
	AMF_UNSUPPORTED,
	AMF_RECORDSET,
	AMF_XML_DOC,
	AMF_TYPED_OBJECT,
	AMF_AVMPLUS,
	AMF_INVALID = 0xff
};

class AMF
{
public:
	AMF();
	~AMF();

	unsigned char* AMF_EncodeString(const char* value, unsigned char* input);

	unsigned char* AMF_EncodeNumber(double value, unsigned char* input);

	unsigned char* AMF_ArrayStart(int items, unsigned char* input);

	unsigned char* AMF_EncodeArrayItem(const char* name, double value, unsigned char* input);

	unsigned char* AMF_EncodeArrayItem(const char* name, bool value, unsigned char* input);

	unsigned char* AMF_EncodeArrayItem(const char* name, const char* value, unsigned char* input);

	unsigned char* AMF_EndObject(unsigned char* input);
	
	unsigned char* AMF_EncodeBoolean(bool val, unsigned char* input);
private:
	unsigned char* AMF_EncodeInt16(int16_t value, unsigned char* input);

	unsigned char* AMF_EncodeInt32(int32_t value, unsigned char* input);
private:
	unsigned char* m_buf;
};