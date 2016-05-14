#include "pump.h"

#include "event_logger.h"
#include "pump_frame.h"
#include "utils.h"

extern EventLoggerClass EventLogger;
extern PumpFrameClass PumpFrame;

PumpClass::PumpClass()
{
	mOverridePin = 0xFF;
	mLevelPin = 0xFF;
	pumpStart = 0;
	pumpRest = 0;
	mStatus = OFF;
}

void PumpClass::init(uint8_t override, uint8_t level)
{
	mOverridePin = override;
	mLevelPin = level;


	pinMode(mOverridePin, OUTPUT);
	pinMode(mLevelPin, INPUT_PULLUP);
}

void PumpClass::start()
{
	mStatus = PUMPING;
	pumpStart = now();

	Serial.print(F("Pump start: "));
	digitalClockDisplay(pumpStart);
}

void PumpClass::rest()
{
	mStatus = RESTING;
	pumpRest = now();

	Serial.print(F("Pump rest: "));
	digitalClockDisplay(pumpRest);
}
void PumpClass::run()
{
	if((mOverridePin == 0xFF) || (mLevelPin == 0xFF))
		return;

	if(timeStatus() != timeSet)
			return;

	bool levelPin = sample();

	if(!inFrame())
	{
		mStatus = OVERRIDE;
		pumpStart = 0;
		pumpRest = 0;
	}

	switch(mStatus)
	{
	case OFF:
		if(levelPin)
		{
			start();
		}
		break;
	case PUMPING:
		digitalWrite(mOverridePin, LOW);
		if((pumpStart + PumpFrame.upTime) < now())
		{
			rest();
		}

		if(!levelPin)
		{
			mStatus = OFF;
		}

		break;
	case RESTING:
		digitalWrite(mOverridePin, HIGH);

		if((pumpRest + PumpFrame.restTime) < now())
		{
			start();
		}

		if(!levelPin)
		{
			mStatus = OFF;
		}
		break;
	case OVERRIDE:
		digitalWrite(mOverridePin, HIGH);

		if(inFrame())
		{
			if(levelPin)
			{
				start();
			}
			else
			{
				mStatus = OFF;
			}
		}
		break;
	}
}

bool PumpClass::sample()
{
	static bool prevState = false;

	bool levelPin = !digitalRead(mLevelPin);

	if(levelPin != prevState)
	{
		prevState = levelPin;

		Event evt(now(), mLevelPin, levelPin);
		EventLogger.log(&evt);
		evt.print();

		//Serial.println("Pump change");
	}

	return levelPin;
}

bool PumpClass::inFrame()
{
	//check if we are in the frame
	uint8_t h = hour();
	if((PumpFrame.startHour <= h) && (h < PumpFrame.endHour))
	{
		return true;
	}
	return false;
}

//void PumpClass::serviceFrame()
//{
//	static bool overrideFlag = 0;
//	if(timeStatus() == timeSet)
//	{
//		//check if we are in the frame
//		uint8_t h = hour();
//		if((PumpFrame.startHour <= h) && (h < PumpFrame.endHour))
//		{
//			if(overrideFlag)
//			{
//				overrideFlag = 0;
//				digitalWrite(5, LOW);
//			}
//
//			//we are in the frame
//			handleOverride();
//		}
//		else
//		{
//			//we are out of operating hours, override the pump
//			digitalWrite(mOverridePin, HIGH);
//			overrideFlag = 1;
//		}
//	}
//}
//
//void PumpClass::handleOverride()
//{
//  //as soon as the pump has to start, start the pumpRunning time out
//  if(!pumpStart && !digitalRead(mLevelPin))
//  {
//    pumpStart = now();
//
//    digitalWrite(mOverridePin, LOW);
//
//    Serial.print(F("Pump start: "));
//    digitalClockDisplay(pumpStart);
//
//    return;
//  }
//
//  if(pumpStart && !pumpRest)
//  {
//    if((pumpStart + PumpFrame.upTime) < now())
//    {
//      pumpRest = now();
//
//      digitalWrite(mOverridePin, HIGH);
//
//      Serial.print(F("Pump rest: "));
//      digitalClockDisplay(pumpRest);
//
//      return;
//    }
//  }
//
//  if(pumpRest && pumpStart)
//  {
//    if((pumpRest + PumpFrame.restTime) < now())
//    {
//      pumpStart = 0;
//      pumpRest = 0;
//
//      digitalWrite(mOverridePin, LOW);
//
//      Serial.println(F("Pump rested"));
//    }
//
//    return;
//  }
//
//  if((pumpStart || pumpRest) && digitalRead(mLevelPin))
//  {
//    pumpStart = 0;
//    pumpRest = 0;
//
//    digitalWrite(mOverridePin, LOW);
//
//    Serial.println(F("Pump stopped"));
//
//    return;
//  }
//
//    digitalWrite(mOverridePin, LOW);
//}

void PumpClass::printStatus()
{
	Serial.println(F("Pump Status:"));

	switch(mStatus)
	{
	case OFF:
		Serial.println(F(" - OFF"));
		break;
	case PUMPING:
		Serial.println(F(" - PUMPING"));
		break;
	case RESTING:
		Serial.println(F(" - RESTING"));
		break;
	case OVERRIDE:
		Serial.println(F(" - OVERRIDE"));
		break;
	}

	Serial.print(F(" - Start: "));
	digitalClockDisplay(pumpStart);

	Serial.print(F(" - Rest: "));
	digitalClockDisplay(pumpRest);
}

PumpClass::~PumpClass()
{
}

PumpClass Pump;

