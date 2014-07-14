#ifndef _CRC_H_
#define _CRC_H_
#include <cyg/kernel/diag.h>

struct sFrameVar
{
	cyg_uint8 frameLen;
	cyg_bool validFrame;
	cyg_bool escapeChar;

	sFrameVar(){validFrame = false; escapeChar = false; frameLen = 0;};
};

class cCrc
{
private:
	static cCrc * __instance;
    static const cyg_uint16 cCrcTable[256];

    static void add_byte(cyg_uint8 byte, cyg_uint8  * frame_ptr, cyg_uint32 * frame_length);

public:
    static cyg_uint8 crc8(cyg_uint8 * data,cyg_uint32 len);
	static cyg_uint8 crc8_update(cyg_uint8 data,cyg_uint8 crc);
	static cyg_uint16 ccitt_crc16(cyg_uint8 data,cyg_uint16 crc_acc);
	static cyg_uint16 ccitt_crc16(cyg_uint8 * data_ptr,cyg_uint32 len);
	static void HDLC_Frame(cyg_uint8 * payload_ptr,
			cyg_uint32 payload_length,
			cyg_uint8 * frame_ptr,
			cyg_uint32 * frame_length);
	static cyg_bool HDLC_pack(cyg_uint8 byte,
			cyg_uint8* buff,
			cyg_uint16* len,
			cyg_uint16 maxBuffLen,
			sFrameVar* var);

	static cyg_uint32 unFrame(cyg_uint8* buff,
			cyg_uint32 len,
			cyg_uint8* returnBuff);

	static const cyg_uint16 INIT_CRC;
    static const cyg_uint16 GOOD_CRC;
	static cCrc & get();

};




#endif //Include Guard
