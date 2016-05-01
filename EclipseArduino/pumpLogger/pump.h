#ifndef PUMP_H_
#define PUMP_H_
#include <Arduino.h>
#include "Time.h"

class PumpClass
{
	time_t pumpStart;
	time_t pumpRest;
	uint8_t mOverridePin;
	uint8_t mLevelPin;
	uint8_t prevState;

	void sample();
	void serviceFrame();
	void handleOverride();

public:
	PumpClass();
	virtual ~PumpClass();

	void init(uint8_t override, uint8_t level);
	void run();
};


extern PumpClass Pump;

#endif /* PUMP_H_ */
