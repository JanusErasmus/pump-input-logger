#ifndef _EVENT_H_
#define _EVENT_H_
#include <cyg/kernel/kapi.h>
#include <time.h>


/* The structure that make up an event for now */
struct sEventData
{
	cyg_uint8 mType;
	cyg_uint32 mSeq;
	cyg_uint8 mPort;
	float mTemp;
	cyg_uint8 mState;
	cyg_uint32 mTime;
	cyg_uint8 mReserved[15]; //add 15 bytes to have 32 bytes in packets to fit in the 0x10000 sector size
	cyg_uint8 mCrc;
	cyg_uint8 mCrcUpdated;
} __attribute__((packed));


class cEvent
{
public:
	enum eEventType
	{
		EVENT_UNKNOWN = 0,
		EVENT_TEMP,
		EVENT_INPUT
	};

private:

	   static cyg_uint32 mLastSeq;
	   sEventData mData;

public:
   void setProcessed();
   cyg_bool isProcessed();
   static void setSeqNumber(cyg_uint32);
   cEvent(cyg_uint8 port, float temp, time_t t);
   cEvent(cyg_uint8 port, cyg_uint8 state, time_t t);
   cEvent();
   void setData(sEventData &d);
   cyg_bool isValid();
   sEventData & getData();

   eEventType getType();
   cyg_uint32 getSeq();
   cyg_uint8 getPort();
   float getTemp();
   cyg_uint32 getTimeStamp();
   cyg_uint8 getState();

   void showEvent();

};

#endif //Include Guard
