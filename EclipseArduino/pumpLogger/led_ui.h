#ifndef LED_UI_H_
#define LED_UI_H_
#include <Arduino.h>

class LedUi
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
	LedUi();
	virtual ~LedUi();

	void init(uint8_t pin);

	void run();

	void setError(){ mState = LED_ERROR; };
	void setIdle(){ mState = LED_HEARTBEAT; };
	void setConnecting(){ mState = LED_CONNECTING; };
};

extern LedUi LED;

#endif /* LED_UI_H_ */
