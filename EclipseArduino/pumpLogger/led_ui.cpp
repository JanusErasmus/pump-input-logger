

#include "led_ui.h"

LedUi::LedUi()
{
	mPin = 0xFF;
	mState = LED_ERROR;
	mCount = 0;
}

void LedUi::init(uint8_t pin)
{
	mPin = pin;

	pinMode(mPin, OUTPUT); //green wire, TOP LED
}

void LedUi::run()
{
	if(mPin == 0xFF)
		return;

	switch(mState)
	{
	case LED_HEARTBEAT:
		heartBeat();
		break;
	case LED_CONNECTING:
		connecting();
		break;
	case LED_ERROR:
		error();
		break;
	}
}

void LedUi::error()
{
	switch(mCount++)
	{
	case 0:
		digitalWrite(mPin, HIGH);
		break;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		break;
	case 6:
		digitalWrite(mPin, LOW);
		break;
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
		break;
	case 12:
		mCount = 0;
		break;

	}
}

void LedUi::heartBeat()
{
	switch(mCount++)
	{
	case 0:
		digitalWrite(mPin, HIGH);
		break;
	case 1:
		digitalWrite(mPin, LOW);
		break;
	case 2:
		digitalWrite(mPin, HIGH);
		break;
	case 3:
		digitalWrite(mPin, LOW);
		break;
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
		break;
	case 12:
		mCount = 0;
		break;
	}
}

void LedUi::connecting()
{
	if(mCount++ > 0)
	{
		digitalWrite(mPin, LOW);
		mCount = 0;
	}
	else
	{
		digitalWrite(mPin, HIGH);
	}
}

LedUi::~LedUi()
{
}

LedUi LED;

