#include "TsPacker.h"
#include <stdint.h>
#include "bits.h"

#define TRANSPORT_PACKET_SIZE 188
#define PAT_PID 0
#ifndef OUR_PROGRAM_NUMBER
#define OUR_PROGRAM_NUMBER 1
#endif
#define OUR_PROGRAM_MAP_PID 0x30

static unsigned int const CRC32[256] = {
	0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
	0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
	0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
	0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
	0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
	0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
	0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
	0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
	0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
	0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
	0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
	0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
	0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
	0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
	0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
	0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
	0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
	0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
	0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
	0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
	0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
	0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
	0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
	0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
	0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
	0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
	0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
	0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
	0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
	0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
	0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
	0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
	0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
	0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
	0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
	0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
	0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
	0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
	0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
	0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
	0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
	0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
	0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
	0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
	0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
	0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
	0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
	0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
	0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
	0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
	0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
	0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
	0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
	0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
	0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
	0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
	0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
	0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
	0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
	0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
	0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
	0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
	0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
	0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

unsigned int calculateCRC(unsigned char const* data, unsigned dataLength, unsigned int initialValue)
{
	unsigned crc = initialValue;

	while (dataLength-- > 0)
	{
		crc = (crc << 8) ^ CRC32[(crc >> 24) ^ (unsigned)(*data++)];
	}

	return crc;
}

TsPacker::TsPacker(const std::wstring& dir) :m_hFile(INVALID_HANDLE_VALUE), m_cb(NULL),
m_usr(nullptr), m_beginTime(0), index(0),
m_lastTime(0)
{
	Reset();
	m_directory = dir;

	CreateDirectory(m_directory.c_str(), NULL);

	pkt = new unsigned char[TRANSPORT_PACKET_SIZE];
}

TsPacker::~TsPacker()
{
	CloseHandle(m_hFile);
	std::wstring fullPath = m_directory + m_currentFile;
	DeleteFile(fullPath.c_str());

	RemoveDirectory(m_directory.c_str());
	delete pkt;
}

void TsPacker::Reset()
{
	pat_counter = 0;
	pmt_counter = 0;
	payload_counter = 0;
}

void TsPacker::deliverPATPacket()
{
	unsigned char* header = pkt;
	*header++ = 0x47; // sync_byte
	*header++ = 0x40; //// transport_error_indicator, payload_unit_start_indicator, transport_priority,
	*header++ = 0;//pid
	*header++ = 0x10 | (pat_counter & 0x0F);
	++pat_counter;

	unsigned char* pat = pkt + 4;

	*pat++ = 0; // pointer_field
	*pat++ = 0; // table_id
	*pat++ = 0xB0; // section_syntax_indicator; 0; reserved, section_length (high)
	*pat++ = 13; // section_length (low)
	*pat++ = 0; *pat++ = 1; // transport_stream_id
	*pat++ = 0xC3; // reserved 11 ; version_number 00001; current_next_indicator 1
	*pat++ = 0; // section_number
	*pat++ = 0; // last_section_number
	*pat++ = OUR_PROGRAM_NUMBER >> 8; *pat++ = OUR_PROGRAM_NUMBER; // program_number
	*pat++ = 0xE0 | (OUR_PROGRAM_MAP_PID >> 8); // reserved 111; program_map_PID (high)
	*pat++ = OUR_PROGRAM_MAP_PID; // program_map_PID (low)

	// Compute the CRC from the bytes we currently have (not including "pointer_field"):
	unsigned int crc = calculateCRC(pkt + 1 + 4, pat - (pkt + 1 + 4));
	*pat++ = crc >> 24; *pat++ = crc >> 16; *pat++ = crc >> 8; *pat++ = crc;

	// Fill in the rest of the packet with padding bytes:
	while (pat < &pkt[TRANSPORT_PACKET_SIZE]) *pat++ = 0xFF;

	DWORD byteWrites;
	WriteFile(m_hFile, pkt, TRANSPORT_PACKET_SIZE, &byteWrites, NULL);
}

void TsPacker::deliverPMTPacket()
{
	unsigned char* header = pkt;
	*header++ = 0x47; // sync_byte
	*header++ = 0x40; //// transport_error_indicator, payload_unit_start_indicator, transport_priority,
	*header++ = OUR_PROGRAM_MAP_PID;//pid
	*header++ = 0x10 | (pmt_counter & 0x0F);
	++pmt_counter;
	// and fill it in:
	unsigned char* pmt = pkt + 4;
	*pmt++ = 0; // pointer_field
	*pmt++ = 2; // table_id
	*pmt++ = 0xB0; // section_syntax_indicator; 0; reserved, section_length (high)
	//unsigned char* section_lengthPtr = pmt; // save for later
	*pmt++ = 0x17;//section_lengthPtr
	*pmt++ = OUR_PROGRAM_NUMBER >> 8; *pmt++ = OUR_PROGRAM_NUMBER; // program_number
	*pmt++ = 0xC1 | ((1 & 0x1F) << 1); // reserved; version_number; current_next_indicator
	*pmt++ = 0; // section_number
	*pmt++ = 0; // last_section_number
	*pmt++ = 0xE0; // reserved; PCR_PID (high)
	*pmt++ = 0xE0; // PCR_PID (low)
	*pmt++ = 0xF0; // reserved; program_info_length (high)
	*pmt++ = 0; // program_info_length (low)

	*pmt++ = 0x1B; //H264
	*pmt++ = 0xE0; // reserved; elementary_pid (high)
	*pmt++ = 0xE0; // elementary_pid (low)
	*pmt++ = 0xF0; // reserved; ES_info_length (high)
	*pmt++ = 0; // ES_info_length (low)

	*pmt++ = 0x0f; //AAC
	*pmt++ = 0xE0; // reserved; elementary_pid (high)
	*pmt++ = 0xE0 + 1; // elementary_pid (low)
	*pmt++ = 0xF0; // reserved; ES_info_length (high)
	*pmt++ = 0; // ES_info_length (low)

	unsigned int crc = calculateCRC(pkt + 1 + 4, pmt - (pkt + 1 + 4));
	*pmt++ = crc >> 24; *pmt++ = crc >> 16; *pmt++ = crc >> 8; *pmt++ = crc;

	while (pmt < &pkt[TRANSPORT_PACKET_SIZE]) *pmt++ = 0xFF;

	DWORD byteWrites;
	WriteFile(m_hFile, pkt, TRANSPORT_PACKET_SIZE, &byteWrites, NULL);
}

void TsPacker::deliverESPacket(unsigned char const* frame, unsigned int frame_size, unsigned int pts, bool iFrame)
{
	/**/
	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		if (!iFrame)
			return;

		CreateNextFile();
		m_beginTime = pts;
	}

	if (pts - m_beginTime >= 2000 && iFrame == true)
	{
		CloseHandle(m_hFile);
		if (m_cb)
		{
			TsFileInfo tf;
			tf.fileDuration = m_lastTime - m_beginTime;
			tf.fileName = m_currentFile;
			tf.index = index - 1;
			m_cb(tf, m_usr);

			//printf("out:%S,duration:%u\n", m_currentFile.c_str(), tf.fileDuration);
		}
		
		CreateNextFile();
		
		m_beginTime = pts;

		Reset();
	}

	m_lastTime = pts;

	DWORD byteWrites;
	if (iFrame)
	{
		deliverPATPacket();
		deliverPMTPacket();
	}
	calcSCR(pts);

	std::basic_string<unsigned char> hdr = make_pes_head(frame_size, pts * 90);
	unsigned int pesSize = hdr.size() + frame_size;
	unsigned char* pes = new unsigned char[pesSize];
	memcpy(pes, hdr.c_str(), hdr.size());
	memcpy(pes + hdr.size(), frame, frame_size);

	unsigned offset = 0;
	int i = 0;
	while (offset < pesSize)
	{
		if (i == 0)/*加上pcr*/
		{
			memset(pkt, 0xff, TRANSPORT_PACKET_SIZE);
			unsigned char* header = pkt;
			*header++ = 0x47;// sync_byte
			*header++ = 0x40;// transport_error_indicator, payload_unit_start_indicator, transport_priority,
			*header++ = 0xE0;//pid
			*header++ = 0x30 | (payload_counter & 0x0F);
			payload_counter++;

			unsigned int filed_legth = 0x07;
			if (pesSize <= TRANSPORT_PACKET_SIZE - 12)
				filed_legth = TRANSPORT_PACKET_SIZE - pesSize - 5;
			else
				filed_legth = 0x07;

			*header++ = filed_legth;//pointer_field
			*header++ = 0x10;/*adptionfiled*/
			/*PCR*/
			unsigned int pcrHigh32Bits = (fSCR.highBit << 31) | (fSCR.remainingBits >> 1);
			unsigned char pcrLowBit = fSCR.remainingBits & 1;
			unsigned char extHighBit = (fSCR.extension & 0x100) >> 8;
			*header++ = pcrHigh32Bits >> 24;
			*header++ = pcrHigh32Bits >> 16;
			*header++ = pcrHigh32Bits >> 8;
			*header++ = pcrHigh32Bits;
			*header++ = (pcrLowBit << 7) | 0x7E | extHighBit;
			*header++ = (unsigned char)fSCR.extension;

			memcpy(pkt + 5 + filed_legth, pes, TRANSPORT_PACKET_SIZE - 5 - filed_legth);
			offset += TRANSPORT_PACKET_SIZE - 5 - filed_legth;
			i = 1;
			WriteFile(m_hFile, pkt, TRANSPORT_PACKET_SIZE, &byteWrites, NULL);
		}
		else
		{
			unsigned int availableSize = pesSize - offset;
			if (availableSize <= TRANSPORT_PACKET_SIZE - 4 - 2)/*最后一个包*/
			{
				memset(pkt, 0xff, TRANSPORT_PACKET_SIZE);
				unsigned char* header = pkt;
				*header++ = 0x47;// sync_byte
				*header++ = 0x20;// transport_error_indicator, payload_unit_start_indicator, transport_priority,
				*header++ = 0xE0;//pid
				*header++ = 0x30 | (payload_counter & 0x0F);
				payload_counter++;
				unsigned int filed_length = TRANSPORT_PACKET_SIZE - 5 - availableSize;
				*header++ = filed_length;//pointer_field
				*header++ = 0x00;

				memcpy(pkt + 5 + filed_length, pes + offset, availableSize);
				offset += availableSize;
				WriteFile(m_hFile, pkt, TRANSPORT_PACKET_SIZE, &byteWrites, NULL);
			}
			else
			{
				unsigned char* header = pkt;
				*header++ = 0x47;// sync_byte
				*header++ = 0x00;// transport_error_indicator, payload_unit_start_indicator, transport_priority,
				*header++ = 0xE0;//pid
				*header++ = 0x10 | (payload_counter & 0x0F);
				payload_counter++;

				memcpy(pkt + 4, pes + offset, TRANSPORT_PACKET_SIZE - 4);
				offset += TRANSPORT_PACKET_SIZE - 4;
				WriteFile(m_hFile, pkt, TRANSPORT_PACKET_SIZE, &byteWrites, NULL);
			}
		}
	}
	delete pes;
}

void TsPacker::calcSCR(unsigned int pts)
{
	fPresentationTime.tv_sec = pts / 1000;
	fPresentationTime.tv_usec = pts % 1000 * 1000;

	fSCR.highBit = ((fPresentationTime.tv_sec * 45000 + (fPresentationTime.tv_usec * 9) / 200) & 0x80000000) != 0;
	fSCR.remainingBits = fPresentationTime.tv_sec * 90000 + (fPresentationTime.tv_usec * 9) / 100;
	fSCR.extension = (fPresentationTime.tv_usec * 9) % 100;
}

std::basic_string<unsigned char> TsPacker::make_pes_head(unsigned int frame_size, unsigned long long _ui64SCR)
{
	std::basic_string<unsigned char> s;

	bits_buffer_t bits;
	unsigned char pes[6 + 3 + 5];
	bits_initwrite(&bits, 14, pes);

	bits_write(&bits, 24, 0x01);/*start code prefix*/
	bits_write(&bits, 8, 0xe0);/*stream_id*/

	if (frame_size + 3 + 5 > 0xffff)
		bits_write(&bits, 16, 0);
	else
		bits_write(&bits, 16, frame_size + 3 + 5);/*pes packet size 指出紧跟在该字段后的描述符的总长度*/
	bits_write(&bits, 2, 0x02);//always set to 10
	bits_write(&bits, 2, 0x00); // pes scrambling control
	bits_write(&bits, 1, 0x00); // pes priority
	bits_write(&bits, 1, 0x00); // data alignement indicator
	bits_write(&bits, 1, 0x00); // copyright
	bits_write(&bits, 1, 0x00);//original or copy

	bits_write(&bits, 2, 0x02); // pts_dts flags
	bits_write(&bits, 1, 0x00); // escr flags
	bits_write(&bits, 1, 0x00); // es rate flag
	bits_write(&bits, 1, 0x00); // dsm trick mode flag
	bits_write(&bits, 1, 0x00); // additional copy info flag
	bits_write(&bits, 1, 0x00); // pes crc flag
	bits_write(&bits, 1, 0x00); // pes extension flag 置'1'时表示PES 分组标题中有扩展字段
	bits_write(&bits, 8, 0x05); //pes header data length 包含在PES 分组标题中的可选字段和任何填充字节所占用的总字节数

	//pts dts flags
	bits_write(&bits, 4, 0x02); // '0010' or '0011'
	bits_write(&bits, 3, (_ui64SCR >> 30) & 0x07);
	bits_write(&bits, 1, 0x01); // marker
	bits_write(&bits, 15, (_ui64SCR >> 15) & 0x7fff);
	bits_write(&bits, 1, 0x01); // marker
	bits_write(&bits, 15, _ui64SCR);
	bits_write(&bits, 1, 0x01); // marker

	//bits_write(&bits, 4, 0x01); // reserved
	//bits_write(&bits, 3, (_ui64SCR >> 30) & 0x07);
	//bits_write(&bits, 1, 0x01); // marker
	//bits_write(&bits, 15, (_ui64SCR >> 15) & 0x7fff);
	//bits_write(&bits, 1, 0x01); // marker
	//bits_write(&bits, 15, _ui64SCR);
	//bits_write(&bits, 1, 0x01); // marker

	s.append(pes, 14);
	return s;
}

DWORD TsPacker::CreateNextFile()
{
	WCHAR fn[32];
	swprintf_s(fn, L"t%02u.ts", index);
	std::wstring fullPath = m_directory + fn;
	m_hFile = CreateFile(fullPath.c_str(), GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		DWORD ret = GetLastError();
		fprintf(stderr, "CreateFile failed:%u\n", ret);
		return ret;
	}

	++index;
	m_currentFile = fn;
	return 0;
}