#ifndef LED_DUI_H_
#define LED_DUI_H_
#include <Arduino.h>

class LEDuiClass
{
	uint8_t mPin;
	uint8_t mCount;

	enum eLEDstate
	{
		LED_ERROR,
		LED_HEARTBEAT,
		LED_CONNECTING,
	};

	eLEDstate mState;

	void error();
	void heartBeat();
	void connecting();

public:
	LEDuiClass();
	virtual ~LEDuiClass();

	void init(uint8_t pin);

	void run();

	void setError(){ mState = LED_ERROR; };
	void setIdle(){ mState = LED_HEARTBEAT; };
	void setConnecting(){ mState = LED_CONNECTING; };
};

extern LEDuiClass LEDui;

#endif /* LED_DUI_H_ */
