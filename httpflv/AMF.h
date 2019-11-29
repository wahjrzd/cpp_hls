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

const std::string strOnMetaData = "onMetaData";
const std::string strDuration = "duration";
const std::string strWidth = "width";
const std::string strHeight = "height";
const std::string strVideodatarate = "videodatarate";
const std::string strFramerate = "framerate";
const std::string strVideocodecid = "videocodecid";
const std::string strAudiodatarate = "audiodatarate";
const std::string strAudiosamperate = "audiosamplerate";
const std::string strAudiosamplesize = "audiosamplesize";
const std::string strStereo = "stereo";
const std::string strAudiocodecid = "audiocodecid";
const std::string strEncoder = "encoder";
const std::string strFilesize = "filesize";

class AMF
{
public:
	AMF();
	~AMF();

	static unsigned char* AMF_EncodeBoolean(unsigned char* input, bool val);

	static unsigned char* AMF_Encodestr(unsigned char* input, unsigned char* str, unsigned int sz);

	static unsigned char* AMF_Encodenum(unsigned char* input, double val);

	template<typename T>
	static unsigned char* AMF_tt(unsigned char* input, const std::string& propName, T val, unsigned int sz)
	{
		*input++ = (propName.size() >> 8) & 0xff;
		*input++ = propName.size() & 0xff;
		memcpy(input, propName.c_str(), propName.size());
		input += propName.size();

		if (std::is_same<T, double>::value)
			input = AMF_Encodenum(input, val);
		else if (std::is_same<T, unsigned char*>::value)
			input = AMF_Encodestr(input, val, sz);
		return input;
	}

	static std::string AMF_EncodeString(const std::string& name);

	static std::string AMF_EncodeNumber(double val);

	static std::string AMF_EncodePropertyWithString(const std::string& propertyName, const std::string& val);

	static std::string AMF_EncodePropertyWithDouble(const std::string& propertyName, double val);

	
};