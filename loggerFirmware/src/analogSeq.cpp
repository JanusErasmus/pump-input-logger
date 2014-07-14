#include <stdio.h>
#include "analogSeq.h"

#include "MCP_rtc.h"

float analogSeq::nan = 0;

analogSeq::analogSeq() : mIndex(0xFF), mSequence(0)
{
}

void analogSeq::set(float val)
{
	if(++mIndex >= SEQ_BUFFER_SIZE)
			mIndex = 0;

	mBuff[mIndex].value = val;
	mBuff[mIndex].sequence = mSequence++;
	mBuff[mIndex].time = cRTC::get()->timeNow();
}

analogSeq::seqSample analogSeq::get()
{
	return mBuff[mIndex];
}

analogSeq::seqSample analogSeq::get(cyg_uint8 seq)
{
	//search for the sequence number in the list and return it
	int idx = 0;
	while(idx < SEQ_BUFFER_SIZE)
	{
		if(mBuff[idx].sequence == seq)
			return mBuff[idx];

		idx++;
	}

	return seqSample(0, nan, 0);
}

analogSeq::seqSample* analogSeq::getBuff()
{
	return mBuff;
}

analogSeq::seqSample::seqSample()
{
	if(nan == 0)
	{
		unsigned long buff[2]={0x7fffffff, 0xffffffff};
		memcpy(&nan, buff,4);
	}

	sequence = 0;
	value = nan;
	time = 0;
}

bool analogSeq::isNaN(float num)
{
	unsigned char* p = (unsigned char*)&num;

	if(p[3] == 0x7F && p[2] == 0xFF && p[1] == 0xFF && p[0] == 0xFF)
		return true;

	if(p[3] == 0xFF && p[2] == 0xFF && p[1] == 0xFF && p[0] == 0xFF)
			return true;

	return false;
}

