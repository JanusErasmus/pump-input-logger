#include <cyg/hal/hal_diag.h>

#include "hobbs_timer.h"
#include "spi_flash.h"
#include "crc.h"
#include "MCP_rtc.h"
#include "pwr_mon.h"
#include "led.h"
#include "event.h"
#include "log.h"

cHobbsTimer* cHobbsTimer::_instance = 0;
cyg_uint8 cHobbsTimer::mPerInc = 0;

void cHobbsTimer::init(cyg_uint32 sectorStart, cyg_uint32 period)
{
	if(!_instance)
	{
		_instance = new cHobbsTimer(sectorStart, period);
	}
}

cHobbsTimer* cHobbsTimer::get()
{
	return _instance;
}

void cHobbsTimer::start()
{
    if(cPwrMon::get()->getPinStat(0))
    {
        diag_printf("HOBB: Alarm0 enabled\n");
        mOnTime0 = cRTC::get()->timeNow();
        cyg_alarm_enable(mAlarmHandle0);
    }
    if(cPwrMon::get()->getPinStat(1))
    {
        diag_printf("HOBB: Alarm1 enabled\n");
        mOnTime1 = cRTC::get()->timeNow();
        cyg_alarm_enable(mAlarmHandle1);
    }
}

cHobbsTimer::cHobbsTimer(cyg_uint32 sectorStart, cyg_uint32 period)
{
	mOnTime0 = 0;
	mOnTime1 = 0;
	mSector = sectorStart;
	mAlarmPeriod = period;
	setAddress();

	//diag_printf("Read: 0x%08X\n", mReadAddr);
	SpiFlash::get()->ReadSpi(mReadAddr, (cyg_uint8*)(&mOnTime), sizeof (sOnTime));
    if(mOnTime.input0 == 0xFFFFFFFF && mOnTime.input1 == 0xFFFFFFFF)
    {
    	mOnTime.total = 0;
        mOnTime.input0 = 0;
        mOnTime.input1 = 0;
        diag_printf("HOBB: reset values\n");
        SpiFlash::get()->flash_erase(mReadAddr);
        mOnTime.mCrc = cCrc::crc8((cyg_uint8*)(&mOnTime), sizeof (sOnTime) - 1);
        SpiFlash::get()->WriteSpi(mReadAddr, (cyg_uint8*)(&mOnTime), sizeof (sOnTime));
    }
    cyg_mutex_init(&mWaitMutex);
    cyg_cond_init(&mWaitCond, &mWaitMutex);
    cyg_mutex_init(&mOnMutex);
    cyg_thread_create(HOBB_PRIOR,
				cHobbsTimer::hobb_thread_func,
				(cyg_addrword_t)this,
				(char *)"HobbsTimer",
				mStack,
				HOBB_STACK_SIZE,
				&mThreadHandle,
				&mThread);
    cyg_thread_resume(mThreadHandle);
    cyg_alarm_create(cyg_real_time_clock(), cHobbsTimer::alarm_handler, 0, &mAlarmHandle0, &mAlarm0);
    cyg_alarm_initialize(mAlarmHandle0, mAlarmPeriod * 500 * 60, mAlarmPeriod * 500 * 60);
    cyg_alarm_disable(mAlarmHandle0);
    cyg_alarm_create(cyg_real_time_clock(), cHobbsTimer::alarm_handler, 1, &mAlarmHandle1, &mAlarm1);
    cyg_alarm_initialize(mAlarmHandle1, mAlarmPeriod * 500 * 60, mAlarmPeriod * 500 * 60);
    cyg_alarm_disable(mAlarmHandle1);
}

void cHobbsTimer::hobb_thread_func(cyg_addrword_t arg)
{
	cHobbsTimer *t = (cHobbsTimer *)arg;
	time_t onTime, now;
	for(;;)
	{
		cyg_cond_wait(&t->mWaitCond);
		cyg_mutex_lock(&t->mOnMutex);

//		diag_printf("HOBB: Add period %d\n", t->mPerInc);

		now = cRTC::get()->timeNow();
		if(now && ( t->mPerInc & 0x01) )
		{
			if(t->mOnTime0)
			{
				onTime = now - t->mOnTime0;
				t->mOnTime0 = now;
				t->incTime(0, onTime);
			}
			else
			{
				t->mOnTime0 = now;
			}
		}

		if(now && ( t->mPerInc & 0x02) )
		{
			if(t->mOnTime1)
			{
				onTime = now - t->mOnTime1;
				t->mOnTime1 = now;
				t->incTime(1, onTime);
			}
			else
			{
				t->mOnTime1 = now;
			}
		}

		t->mPerInc = 0;
		cyg_mutex_unlock(&t->mOnMutex);
	}
}

void cHobbsTimer::setAddress()
{
	cyg_uint32 addr = mSector;
	sOnTime onTime;
	cyg_uint8 crc;

	do
	{
		cLED::get()->toggleLED();

		//diag_printf("Read: 0x%08X, ", addr);
		SpiFlash::get()->ReadSpi(addr, (cyg_uint8*)&onTime, sizeof(sOnTime));
		//diag_printf("i1: 0x%08X, ", onTime.input0);
		//diag_printf("i2: 0x%08X, ", onTime.input1);
		//diag_printf("t: 0x%08X, ", onTime.total);

		crc = cCrc::crc8((cyg_uint8 *)&onTime,sizeof(sOnTime));
		//diag_printf("crc: 0x%02X\n", crc);
		if(crc)
		{
			break;
		}

		addr += sizeof(sOnTime);
		if( (addr + sizeof(sOnTime)) > ( mSector + SpiFlash::get()->GetSectSize()) )
		{
			break;
		}

	}while(!crc);

	mReadAddr = addr - sizeof(sOnTime);

	if(mReadAddr < mSector)
		mReadAddr = mSector;

	diag_printf("HOBB: ReadAddr: 0x%08X\n", mReadAddr);
}

cyg_uint32 cHobbsTimer::getTime(cyg_uint8 input)
{
	switch(input)
	{
		case 0:
			return mOnTime.input0;
		case 1:
			return mOnTime.input1;
		case 2:
			return mOnTime.total;

		default:
			return 0;
	}

}

void cHobbsTimer::incTime(cyg_uint8 input, cyg_uint32 time)
{
	bool flag = false;

	switch(input)
	{
		case 0:
			mOnTime.total += time;
			mOnTime.input0 += time;
			flag = true;
			break;
		case 1:
			mOnTime.total += time;
			mOnTime.input1 += time;
			flag = true;
			break;

		default:
			break;
	}

	if(flag)
	{
		mReadAddr += sizeof(sOnTime);
		if(mReadAddr >= (mSector + SpiFlash::get()->GetSectSize()))
		{
			mReadAddr = mSector;
			SpiFlash::get()->flash_erase(mReadAddr);
		}
		if(!checkSpace())
		{
			diag_printf("No space @ 0x%08X\n", mReadAddr);
			mReadAddr = mSector;
			SpiFlash::get()->flash_erase(mReadAddr);
		}

		//diag_printf("HOBB: Write: 0x%08X\n", mReadAddr);
		mOnTime.mCrc = cCrc::crc8((cyg_uint8 *)&mOnTime,sizeof(sOnTime)-1);
		if(!SpiFlash::get()->WriteSpi(mReadAddr, (cyg_uint8*)&mOnTime, sizeof(sOnTime)))
		{
			diag_printf("Hobbs flash write error: 1\n");
			cEvent e(99, (cyg_uint8)1, cRTC::get()->timeNow());
			e.showEvent();
			cLog::get()->logEvent(&e);

			mReadAddr = mSector;
			SpiFlash::get()->flash_erase(mReadAddr);
			SpiFlash::get()->WriteSpi(mReadAddr, (cyg_uint8*)&mOnTime, sizeof(sOnTime));
		}

		diag_printf("HOBB: increment input%d %d s\n", input, time);
		//showTimes();
	}
}

void cHobbsTimer::setTime(cyg_uint8 input, cyg_uint32 time)
{
	bool flag = false;

	switch(input)
	{
		case 0:
			mOnTime.input0 = time;
			flag = true;
			break;
		case 1:
			mOnTime.input1 = time;
			flag = true;
			break;

		default:
			break;
	}

	if(flag)
	{
		mReadAddr += sizeof(sOnTime);
		if(mReadAddr >= (mSector + SpiFlash::get()->GetSectSize()))
		{
			mReadAddr = mSector;
			SpiFlash::get()->flash_erase(mReadAddr);
		}
		if(!checkSpace())
		{
			diag_printf("No space @ 0x%08X\n", mReadAddr);
			mReadAddr = mSector;
			SpiFlash::get()->flash_erase(mReadAddr);
		}

		//diag_printf("HOBB: Write: 0x%08X\n", mReadAddr);
		mOnTime.mCrc = cCrc::crc8((cyg_uint8 *)&mOnTime,sizeof(sOnTime)-1);
		if(!SpiFlash::get()->WriteSpi(mReadAddr, (cyg_uint8*)&mOnTime, sizeof(sOnTime)))
		{
			diag_printf("Hobbs flash set time error: 3\n");
			cEvent e(99, (cyg_uint8)3, cRTC::get()->timeNow());
			e.showEvent();
			cLog::get()->logEvent(&e);

			mReadAddr = mSector;
			SpiFlash::get()->flash_erase(mReadAddr);
			SpiFlash::get()->WriteSpi(mReadAddr, (cyg_uint8*)&mOnTime, sizeof(sOnTime));
		}

		diag_printf("HOBB: updated input%d %d s\n", input, time);
		//showTimes();
	}
}

void cHobbsTimer::showTimes()
{
	cyg_uint32 hour, min, sec;

	hour = mOnTime.total / 3600;
	sec = mOnTime.total - (hour * 3600);
	min = sec / 60;
	//diag_printf("%ds\n", mOnTime.input0);
	diag_printf("TOTAL: %02dh%02dmin\n", hour, min);

	hour = mOnTime.input0 / 3600;
	sec = mOnTime.input0 - (hour * 3600);
	min = sec / 60;
	//diag_printf("%ds\n", mOnTime.input0);
	diag_printf("Input0: %02dh%02dmin\n", hour, min);

	hour = mOnTime.input1 / 3600;
	sec = mOnTime.input1 - (hour * 3600);
	min = sec / 60;
	//diag_printf("%ds\n", mOnTime.input1);
	diag_printf("Input1: %02dh%02dmin\n", hour, min);
}

void cHobbsTimer::alarm_handler(cyg_handle_t alarm, cyg_addrword_t data)
{
	cyg_uint8 num = (cyg_uint8)data;
	cyg_mutex_lock(&_instance->mOnMutex);
//	diag_printf("HOBB: alarm%d\n", num);
//	time_t t = cRTC::get()->time();
//	diag_printf(ctime(&t));

	mPerInc |= 1 << num;
	cyg_cond_signal(&_instance->mWaitCond);

	cyg_mutex_unlock(&_instance->mOnMutex);
}

void cHobbsTimer::setPortState(cyg_uint8 input, bool state)
{
	switch(input)
		{
			case 0:
				if(state)
				{
					 mOnTime0 = cRTC::get()->timeNow();
					cyg_alarm_enable(mAlarmHandle0);
				}
				else
				{
					cyg_alarm_disable(mAlarmHandle0);
				}
				break;
			case 1:
				if(state)
				{
					 mOnTime1 = cRTC::get()->timeNow();
					cyg_alarm_enable(mAlarmHandle1);
				}
				else
				{
					cyg_alarm_disable(mAlarmHandle1);
				}
				break;

			default:
				break;
		}
}

bool cHobbsTimer::checkSpace()
{
	bool stat = true;

	cyg_uint8 buff[sizeof(sOnTime)];
	SpiFlash::get()->ReadSpi(mReadAddr, buff, sizeof(sOnTime));

	for (cyg_uint8 k = 0; k < sizeof(sOnTime); ++k)
	{
		if(buff[k] != 0xFF)
		{
			stat = false;
			break;
		}
	}

	if(!stat)
	{
		SpiFlash::get()->ReadSpi(mReadAddr, buff, sizeof(sOnTime));
		diag_printf("Unsuspected data, error 2\n");
		diag_dump_buf(buff, sizeof(sOnTime));

		cEvent e(99, (cyg_uint8)2, cRTC::get()->timeNow());
		e.showEvent();
		cLog::get()->logEvent(&e);
	}

	return stat;
}

cHobbsTimer::~cHobbsTimer()
{
}








