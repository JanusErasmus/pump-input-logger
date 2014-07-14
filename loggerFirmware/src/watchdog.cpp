#include <cyg/kernel/diag.h>

#include "watchdog.h"

cWatchDog* cWatchDog::_instance = 0;

void cWatchDog::init()
{
	if(!_instance)
	{
		_instance = new cWatchDog();
	}
}

cWatchDog* cWatchDog::get()
{
	return _instance;
}

cWatchDog::cWatchDog() : mKickCnt(0)
{
	cyg_thread_create(WD_PRIOR,
			cWatchDog::wd_thread_func,
			(cyg_addrword_t)this,
			(char *)"Watchdog",
			mStack,
			WD_STACK_SIZE,
			&mThreadHandle,
			&mThread);

	cyg_thread_resume(mThreadHandle);

	//Start watchdog timer
	//HAL_WRITE_UINT32(CYGHWR_HAL_STM32_ + CYGHWR_HAL_STM32_WWDG_CFR, 0x1FF);
	//HAL_WRITE_UINT32(CYGHWR_HAL_STM32_ + CYGHWR_HAL_STM32_WWDG_CR, 0xFF);



}

void cWatchDog::wd_thread_func(cyg_addrword_t arg)
{
	cWatchDog *t = (cWatchDog *)arg;

	cyg_uint32 reg32;

	//switch on LSI
	HAL_WRITE_UINT32(CYGHWR_HAL_STM32_RCC + CYGHWR_HAL_STM32_RCC_CSR, 0x01);
	do
	{
		HAL_READ_UINT32(CYGHWR_HAL_STM32_RCC + CYGHWR_HAL_STM32_RCC_CSR, reg32);
		//diag_printf("RCC_CSR: 0x%08X\n", reg32);
		//cyg_thread_delay(100);
	}while(!(reg32 & 0x02));
//	diag_printf("RCC_CSR: 0x%08X\n", reg32);

	//diag_printf("Watchdog started\n");
	//Start watchdog timer
	HAL_WRITE_UINT32(CYGHWR_HAL_STM32_IWDG + CYGHWR_HAL_STM32_IWDG_KR, 0xCCCC);

	HAL_WRITE_UINT32(CYGHWR_HAL_STM32_IWDG + CYGHWR_HAL_STM32_IWDG_KR, 0x5555);

	do
	{
		HAL_READ_UINT32(CYGHWR_HAL_STM32_IWDG + CYGHWR_HAL_STM32_IWDG_SR, reg32);
		//diag_printf("IWD_SR: 0x%08X\n", reg32);
		//cyg_thread_delay(100);
	}while(reg32 & 0x01);
	//diag_printf("IWD_SR -> 0x%08X\n", reg32);

	HAL_WRITE_UINT32(CYGHWR_HAL_STM32_IWDG + CYGHWR_HAL_STM32_IWDG_PR, 0x07);

	do
	{
		HAL_READ_UINT32(CYGHWR_HAL_STM32_IWDG + CYGHWR_HAL_STM32_IWDG_SR, reg32);
		//diag_printf("IWD_SR: 0x%08X\n", reg32);
		//cyg_thread_delay(100);
	}while(reg32 & 0x01);
//	diag_printf("IWD_SR: 0x%08X\n", reg32);

//	HAL_READ_UINT32(CYGHWR_HAL_STM32_IWDG + CYGHWR_HAL_STM32_IWDG_PR, reg32);
//	diag_printf("IWD_PR: 0x%08X\n", reg32);
//	HAL_READ_UINT32(CYGHWR_HAL_STM32_IWDG + CYGHWR_HAL_STM32_IWDG_RLR, reg32);
//	diag_printf("IWD_RLR: 0x%08X\n", reg32);
	cyg_thread_delay(100);


	for(;;)
	{
		t->run();
	}
}

void cWatchDog::run()
{
	bool kickFlag = true;

	if(!mKickCnt)
	{
		cyg_thread_delay(500);
		return;
	}

	//If any of the registered kickers was not reset, don't kick the dog
	for(cyg_uint8 k = 0; k < mKickCnt; k++)
	{
		if(!mKickers[k])
			continue;

		mKickers[k]->sleep();

		if(mKickers[k]->isSleeping())
		{
			kickFlag = false;
		}
	}

	if(kickFlag)
	{
		//HAL_WRITE_UINT32(CYGHWR_HAL_STM32_ + CYGHWR_HAL_STM32_WWDG_CR, 0xFF);
		HAL_WRITE_UINT32(CYGHWR_HAL_STM32_IWDG + CYGHWR_HAL_STM32_IWDG_KR, 0xAAAA);
	}

	cyg_thread_delay(500);
}

bool cWatchDog::registerKicker(wdKicker *kicker)
{
	if(!kicker)
		return false;

	if(mKickCnt + 1 > KICK_COUNT)
		return false;

	mKickers[mKickCnt++] = kicker;

	return true;
}

cWatchDog::~cWatchDog()
{
}

wdKicker::wdKicker(cyg_int32 time) : mTimeOut(time), mCount(time), mEnabled(true)
{
	cWatchDog::init();
	cWatchDog::get()->registerKicker(this);
}

void wdKicker::sleep()
{
	if(mEnabled)
		mCount--;

	if(mCount < 10)
	{
//		HAL_WRITE_UINT32(CYGHWR_HAL_STM32_IWDG + CYGHWR_HAL_STM32_IWDG_KR, 0xAAAA);
		diag_printf("WATCHDOG %p went to sleep - %d\n", this, mCount);
	}
}

void wdKicker::reset()
{
	mCount = mTimeOut;
}

bool wdKicker::isSleeping()
{
	return (mCount < 0);
}

void wdKicker::enable()
{
	mEnabled = true;
}

void wdKicker::disable()
{
	mEnabled = false;
}

wdKicker::~wdKicker()
{
}





