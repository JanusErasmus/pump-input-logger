#include <cyg/kernel/diag.h>
#include <stdio.h>

#include "led.h"
#include "input_port.h"
#include "nvm.h"

cLED* cLED::__instance = NULL;

void cLED::init(ledPins_s* pinNumbers, cyg_uint8 pinCount)
{
	diag_printf("Initialising cLED\n");
	if(!__instance)
	{
		__instance = new cLED(pinNumbers, pinCount);
	}
}

cLED* cLED::get()
{
	return __instance;
}

cLED::cLED(ledPins_s* pinNumbers, cyg_uint8 pinCount) : mLEDCnt(pinCount)
{
	mLEDenabled = true;

	mLEDList = new ledPins_s[mLEDCnt];
	memcpy(mLEDList, pinNumbers, sizeof(ledPins_s) * mLEDCnt);

	cyg_mutex_init(&mStatMutex);
	mStatus = idle;

	setupPorts(mLEDList, mLEDCnt);

	animateTest();

	if(cInput::get())
	{
		//Set LED according to input
		for(int k = 0; k < cInput::get()->getPortCount(); k++)
		{
			cyg_uint8 stat = cInput::get()->getPortState(k);
			showIO(k + 1, stat);
		}
	}

	mWatchDog = new wdKicker(60);

	cyg_thread_create(LED_PRIOR,
			cLED::led_thread,
			(cyg_addrword_t)this,
			(char *)"heartBeat",
			mLEDStack,
			LED_STACK_SIZE,
			&mLEDThreadHandle,
			&mLEDThread);
	cyg_thread_resume(mLEDThreadHandle);

}

void cLED::setupPorts(ledPins_s* ports, cyg_uint8 count)
{
	cyg_uint32 port, pin, pinspec;

	for (int k = 0; k < count; k++)
	{
		for (int out = 0; out < 2; out++)
		{
			cyg_uint32 gpio_num = 0xFF;
			if(out == 0)
				gpio_num = ports[k].greenPin;
			else if (out == 1)
				gpio_num = ports[k].redPin;

			if(gpio_num == 0xFF)
				continue;

			pin = gpio_num & 0xF;
			port = gpio_num >> 4;
			//diag_printf("Setup led on port %d pin %d\n", port, pin);

			// Generate the pin setup specification and configure it.
			pinspec = STM32_GPIO_PINSPEC (port, pin, OUT_2MHZ, GPOPP);
			CYGHWR_HAL_STM32_GPIO_SET (pinspec);
		}

	}
}

void cLED::indicate(eLEDstatus stat)
{
	cyg_mutex_lock(&mStatMutex);
	mStatus = stat;
	cyg_mutex_unlock(&mStatMutex);
}

void cLED::led_thread(cyg_addrword_t args)
{
	diag_printf("LED started\n");


	cLED* _this = (cLED*)args;

	for(;;)
	{

		_this->animate();

		_this->mWatchDog->reset();
	}
}

void cLED::animate()
{
	cyg_uint8 stat;

	cyg_mutex_lock(&mStatMutex);
	stat = mStatus;
	cyg_mutex_unlock(&mStatMutex);

	//Only animate walk out when disabled
	if(!mLEDenabled && stat != walkOut)
	{
		setLED(0,off);
		cyg_thread_delay(500);
		return;
	}

	switch(stat)
	{

	case idle:
		animateIdle();
		break;

		//fast flicker
	case connecting:
	case transfering:
	{
		animateFlicker(green);
	}
	break;

	case alarm:
	{
		animateFlicker(red);
	}
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
		animateError(stat);
	}
	break;
	case walkOut:
		animateWalkout();
		break;
	}

}

void cLED::animateWalkout()
{
	setLED(0, green);
	cyg_thread_delay(200);
	setLED(0, red);
	cyg_thread_delay(200);

}

void cLED::animateFlicker(eLEDcolor c)
{
	setLED(0, c);
	cyg_thread_delay(50);
	setLED(0, off);
	cyg_thread_delay(50);
}

void cLED::animateError(cyg_uint8 error)
{

	for(cyg_uint8 k = 0; k < error; k++)
	{
		setLED(0, off);
		cyg_thread_delay(200);
		setLED(0, red);
		cyg_thread_delay(200);
	}
	setLED(0, off);
	cyg_thread_delay(500);

}

void cLED::animateIdle()
{
	eLEDcolor c1, c2;

	if(1) //XXX IDLE COLOUR
	{
		c1 = off;
		c2 = green;
	}
	else
	{
		c1 = off;
		c2 = red;
	}


	setLED(0, c1);
	cyg_thread_delay(30);
	setLED(0, c2);
	cyg_thread_delay(120);
	setLED(0, c1);
	cyg_thread_delay(30);
	setLED(0, c2);
	cyg_thread_delay(800);
}

void cLED::showIO(cyg_uint8 num, bool stat)
{
	//only handle the IO LED's
	if(num >= mLEDCnt || num == 0)
		return;

	//diag_printf("Set led %d : %d - nvm:%d\n", num, stat, cNVM::get()->getInputStat(num));
	//Status LED is LED[0]
	if(stat == cNVM::get()->getInputStat(num - 1))
		setLED(num, green);
	else
		setLED(num, red);
}

void cLED::animateTest()
{
	for (int k = 0; k < mLEDCnt; k++)
		setLED(k,green);

	cyg_thread_delay(200);

	for (int k = 0; k < mLEDCnt; k++)
		setLED(k,red);

	cyg_thread_delay(200);
}

void cLED::setLED(cyg_uint8 led, eLEDcolor color)
{
	if(led >= mLEDCnt)
		return;

	cyg_uint32 port, pin;
	cyg_uint32 greenPinspec = 0;
	cyg_uint32 redPinspec = 0;

	if(mLEDList[led].greenPin != 0xFF)
	{
		pin = mLEDList[led].greenPin & 0xF;
		port = mLEDList[led].greenPin >> 4;
		greenPinspec = STM32_GPIO_PINSPEC (port, pin, OUT_2MHZ, GPOPP);
	}

	if(mLEDList[led].redPin != 0xFF)
	{
		pin = mLEDList[led].redPin & 0xF;
		port = mLEDList[led].redPin >> 4;
		redPinspec = STM32_GPIO_PINSPEC (port, pin, OUT_2MHZ, GPOPP);
	}

	cyg_uint8 state = 0;
	if(color == green)
		state = 1;

	if(greenPinspec && redPinspec)
	{
		switch(color)
		{
		case green:
			CYGHWR_HAL_STM32_GPIO_OUT (greenPinspec, 1);
			CYGHWR_HAL_STM32_GPIO_OUT (redPinspec, 0);
			break;

		case red:
			CYGHWR_HAL_STM32_GPIO_OUT (greenPinspec, 0);
			CYGHWR_HAL_STM32_GPIO_OUT (redPinspec, 1);
			break;

		case off:
			CYGHWR_HAL_STM32_GPIO_OUT (greenPinspec, 0);
			CYGHWR_HAL_STM32_GPIO_OUT (redPinspec, 0);
			break;
		}
	}
	else if (greenPinspec)
	{
		CYGHWR_HAL_STM32_GPIO_OUT (greenPinspec, state);
	}
	else if (redPinspec)
	{
		CYGHWR_HAL_STM32_GPIO_OUT (redPinspec, state);
	}

}
