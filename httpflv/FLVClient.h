#pragma once
#include <string>
#include <queue>
#include <WinSock2.h>

class FLVClient
{
public:
	FLVClient();
	~FLVClient();

	void HasNewFLVTag(const std::basic_string<std::uint8_t>& data);
	std::basic_string<std::uint8_t> GetTagData();

	bool newClient;
private:
	std::queue< std::basic_string<std::uint8_t>> tagQueue;
	CRITICAL_SECTION m_queueLock;
	CONDITION_VARIABLE m_queueCondition;
};