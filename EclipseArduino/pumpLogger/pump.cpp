#include "pump.h"

#include "event_logger.h"
#include "pump_frame.h"
#include "utils.h"

extern EventLoggerClass EventLogger;
extern PumpFrameClass PumpFrame;

PumpClass::PumpClass()
{
	prevState = false;
	mOverridePin = 0xFF;
	mLevelPin = 0xFF;
	pumpStart = 0;
	pumpRest = 0;
}

void PumpClass::init(uint8_t override, uint8_t level)
{
	mOverridePin = override;
	mLevelPin = level;


	pinMode(mOverridePin, OUTPUT);
	pinMode(mLevelPin, INPUT_PULLUP);
}

void PumpClass::run()
{
	if((mOverridePin == 0xFF) || (mLevelPin == 0xFF))
		return;

	sample();
	serviceFrame();
}

void PumpClass::sample()
{
	uint8_t pinState = !digitalRead(mLevelPin);

	if(pinState != prevState)
	{
		prevState = pinState;

		if(timeStatus() == timeSet)
		{
			Event evt(now(), mLevelPin, pinState);
			EventLogger.log(&evt);
			evt.print();

			//Serial.println("Pump change");
		}
		else
		{
			Serial.println("Change NOT logged: no time");
		}
	}
}

void PumpClass::serviceFrame()
{
	static bool overrideFlag = 0;
	if(timeStatus() == timeSet)
	{
		//check if we are in the frame
		uint8_t h = hour();
		if((PumpFrame.startHour <= h) && (h < PumpFrame.endHour))
		{
			if(overrideFlag)
			{
				overrideFlag = 0;
				digitalWrite(5, LOW);
			}

			//we are in the frame
			handleOverride();
		}
		else
		{
			//we are out of operating hours, override the pump
			digitalWrite(mOverridePin, HIGH);
			overrideFlag = 1;
		}
	}
}

void PumpClass::handleOverride()
{
  //as soon as the pump has to start, start the pumpRunning time out
  if(!pumpStart && !digitalRead(mLevelPin))
  {
    pumpStart = now();

    digitalWrite(mOverridePin, LOW);

    Serial.print("Pump start: ");
    digitalClockDisplay(pumpStart);

    return;
  }

  if(pumpStart && !pumpRest)
  {
    if((pumpStart + PumpFrame.upTime) < now())
    {
      pumpRest = now();

      digitalWrite(mOverridePin, HIGH);

      Serial.print("Pump rest: ");
      digitalClockDisplay(pumpRest);

      return;
    }
  }

  if(pumpRest && pumpStart)
  {
    if((pumpRest + PumpFrame.restTime) < now())
    {
      pumpStart = 0;
      pumpRest = 0;

      digitalWrite(mOverridePin, LOW);

      Serial.println("Pump rested");
    }

    return;
  }

  if((pumpStart || pumpRest) && digitalRead(mLevelPin))
  {
    pumpStart = 0;
    pumpRest = 0;

    digitalWrite(mOverridePin, LOW);

    Serial.println("Pump stopped");

    return;
  }

    digitalWrite(mOverridePin, LOW);
}

PumpClass::~PumpClass()
{
}

PumpClass Pump;

