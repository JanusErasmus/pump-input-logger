#include <cyg/kernel/diag.h>
#include <stdio.h>
#include "event.h"
#include "nvm.h"
#include "crc.h"

cyg_uint32 cEvent::mLastSeq = 0;

void cEvent::setSeqNumber(cyg_uint32 s)
{
   mLastSeq = s;
   diag_printf("EVT: Setting seq to 0x%08X\n",s);
}

cEvent::cEvent()
{
	mData.mType = EVENT_UNKNOWN;
}

cEvent::cEvent(cyg_uint8 port, float temp, time_t t)
{
	mData.mType = EVENT_TEMP;
	mData.mSeq = mLastSeq++;
	mData.mTime = t;
	mData.mPort = port;
	mData.mTemp = temp;
	mData.mState = 0;


	mData.mCrc = cCrc::crc8((cyg_uint8 *)&mData,sizeof(mData)-2);
	mData.mCrcUpdated = 0xFF;
}

cEvent::cEvent(cyg_uint8 port, cyg_uint8 state, time_t t)
{
	//diag_printf("New input change event\n");
	mData.mType = EVENT_INPUT;
	mData.mSeq = mLastSeq++;
	mData.mTime = t;
	mData.mPort = port;
	mData.mTemp = 0;
	mData.mState = state;


	mData.mCrc = cCrc::crc8((cyg_uint8 *)&mData,sizeof(mData)-2);
	mData.mCrcUpdated = 0xFF;
}

void cEvent::setData(sEventData & d)
{
   mData = d;
}


void cEvent::showEvent()
{
	if(mData.mType == EVENT_TEMP)
	{
		printf("SEQ[0x%04X]\t", mData.mSeq);
		printf("P[%d]\t", mData.mPort);
		printf("Value: %.1f\t", mData.mTemp);
		printf(ctime((time_t*)&mData.mTime));
	}

	if(mData.mType == EVENT_INPUT)
	{
		printf("SEQ[0x%04X]\t", mData.mSeq);
		printf("P[%d]\t", mData.mPort);
		printf("State: %d\t", mData.mState);
		printf(ctime((time_t*)&mData.mTime));
	}
}



sEventData & cEvent::getData()
{
   return mData;
}

cyg_bool cEvent::isValid()
{
   cyg_uint8 crc = cCrc::crc8((cyg_uint8 *)&mData,sizeof(mData)-1);
   if(!crc)
   {
      return true;
   }

   //diag_printf("EVT: First CRC 0x%02X\n",crc);
   crc = cCrc::crc8((cyg_uint8 *)&mData,sizeof(mData));
   if(!crc)
   {
      return true;
   }

   //diag_printf("EVT: Second CRC 0x%02X\n",crc);

   return false;
}

cEvent::eEventType cEvent::getType()
{
	return (eEventType)mData.mType;
}

cyg_uint32 cEvent::getSeq()
{
   return  mData.mSeq;
}

cyg_uint32 cEvent::getTimeStamp()
{
   return mData.mTime;
}

cyg_uint8 cEvent::getPort()
{
   return mData.mPort;
}

float cEvent::getTemp()
{
   return mData.mTemp;
}

cyg_uint8 cEvent::getState()
{
   return mData.mState;
}

cyg_bool cEvent::isProcessed()
{
   return (cCrc::crc8((cyg_uint8 *)&mData,sizeof(mData))) ? (false) : (true);
}

void cEvent::setProcessed()
{
   mData.mCrcUpdated = cCrc::crc8((cyg_uint8 *)&mData,sizeof(mData)-1);
}

