#pragma once
#include <string>
#include <WinSock2.h>
#define PID_TABLE_SIZE 256

struct TsFileInfo
{
	std::wstring fileName;
	unsigned int fileDuration;//∫¡√Î
	unsigned int index;
};

typedef unsigned(*TSCallback)(TsFileInfo& f, void *arg);

class TsPacker
{
public:
	TsPacker(const std::wstring& dir);
	~TsPacker();
	void deliverESPacket(unsigned char const* frame, unsigned int frame_size, unsigned int pts, bool iFrame);
	void Reset();

	void SetCallback(TSCallback cb, void* arg)
	{
		m_cb = cb;
		m_usr = arg;
	}

	class SCR
	{
	public:
		SCR()
		{
			highBit = 0;
			remainingBits = 0;
			extension = 0;
		}
		unsigned char highBit;
		unsigned int remainingBits;
		unsigned short extension;
	};
private:
	void deliverPATPacket();
	void deliverPMTPacket();

	DWORD CreateNextFile();

	void calcSCR(unsigned int pts);
	std::basic_string<unsigned char> make_pes_head(unsigned int frame_size, unsigned long long _ui64SCR);
private:
	HANDLE m_hFile;

	unsigned char* pkt;
	unsigned char pat_counter;
	unsigned char pmt_counter;
	unsigned char payload_counter;
	timeval fPresentationTime;
	SCR fSCR;
private:
	std::wstring m_directory;
	TSCallback m_cb;
	void* m_usr;
	unsigned int m_beginTime;
	unsigned int m_lastTime;
	unsigned int index;
	std::wstring m_currentFile;
};

unsigned int calculateCRC(unsigned char const* data, unsigned dataLength, unsigned int initialValue = 0xFFFFFFFF);