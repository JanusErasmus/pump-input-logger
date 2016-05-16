#ifndef PUMP_H_
#define PUMP_H_
#include <Arduino.h>
#include "Time.h"

class PumpClass
{
	enum ePumpState
	{
		OFF,
		PUMPING,
		RESTING,
		OVERRIDE
	}mStatus;

	time_t pumpStart;
	time_t pumpRest;

	uint8_t mOverridePin;
	uint8_t mLevelPin;

	bool sample();

	bool inFrame();
	void start();
	void rest();

public:
	PumpClass();
	virtual ~PumpClass();

	void init(uint8_t override, uint8_t level);
	void run();

	void printStatus();
};


extern PumpClass Pump;

#endif /* PUMP_H_ */
