#include <cyg/kernel/diag.h>
#include <cyg/kernel/kapi.h>
#include "var_io.h"

#include "definitions.h"
#include "led.h"

cLED* cLED::_instance = 0;

void cLED::init()
{
	if(!_instance)
	{
		_instance = new cLED();
	}
}

cLED* cLED::get()
{
	return _instance;
}

cLED::cLED()
{
	setRed();
	mStatus = idle;
	mStarted = false;

	cyg_mutex_init(& mStatMutex);
}

void cLED::start()
{
	mWatchDog = new wdKicker(30);
	diag_printf("WATCHDOG %p for LED\n", mWatchDog);

	mStarted = true;
}

void cLED::animate()
{
	if(!mStarted)
		return;

	cyg_uint8 status;
	cyg_mutex_lock(&mStatMutex);
	status = mStatus;
	cyg_mutex_unlock(&mStatMutex);

	switch(status)
	{
		//heart beat flicker with short count according to status
		case idle:
		{
			set_pinH(mLED_h);
			set_pinL(mLED_l);
			cyg_thread_delay(50);
			set_pinL(mLED_h);
			set_pinL(mLED_l);
			cyg_thread_delay(50);
			set_pinH(mLED_h);
			set_pinL(mLED_l);
			cyg_thread_delay(50);
			set_pinL(mLED_h);
			set_pinL(mLED_l);
			cyg_thread_delay(50);

			set_pinH(mLED_h);
			set_pinL(mLED_l);
			cyg_thread_delay(500);
		}
		mWatchDog->reset();
		break;

		case waitNetwork:
		case SIMerr:
		case PINerr:
		case PUKerr:
		case LinkErr:
		case invalidFrame:
		case noReply:
		case couldNotPwrModem:
		{
			for(cyg_uint8 k = 0; k < status; k++)
			{
				set_pinL(mLED_h);
				set_pinL(mLED_l);
				cyg_thread_delay(200);
				set_pinH(mLED_h);
				set_pinL(mLED_l);
				cyg_thread_delay(200);
			}
			set_pinL(mLED_h);
			set_pinL(mLED_l);
			cyg_thread_delay(500);
		}
		mWatchDog->reset();
		break;

		//fast flicker
		case connecting:
		case transfering:
		{
			set_pinH(mLED_h);
			set_pinL(mLED_l);
			cyg_thread_delay(50);
			set_pinL(mLED_h);
			set_pinL(mLED_l);
			cyg_thread_delay(50);
		}
		mWatchDog->reset();
			break;
	}
}

void cLED::toggleLED()
{
	static bool flag = false;

	if(flag)
	{
		flag = 0;
		set_pinH(mLED_h);
		set_pinL(mLED_l);
	}
	else
	{
		flag = 1;
		set_pinH(mLED_l);
		set_pinL(mLED_h);
	}
}

void cLED::indicate(eLEDstatus stat)
{
	cyg_mutex_lock(&mStatMutex);
	switch(stat)
	{
		case connecting:
		case waitNetwork:
		case SIMerr:
		case PINerr:
		case PUKerr:
		case LinkErr:
		case invalidFrame:
		case noReply:
		case couldNotPwrModem:
			setRed();
			break;

		case  transfering:
		case idle:
			setGreen();
			break;
	}

	mStatus = stat;
	cyg_mutex_unlock(&mStatMutex);
}

cLED::eLEDstatus cLED::getStatus()
{
	return mStatus;
}

void cLED::setGreen()
{
	mLED_h = LED_G;
	mLED_l = LED_R;
}

void cLED::setRed()
{
	mLED_h = LED_R;
	mLED_l = LED_G;
}

cLED::~cLED()
{
}

