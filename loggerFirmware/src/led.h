#ifndef LED_H_
#define LED_H_

#include "watchdog.h"

class cLED
{
public:
	enum eLEDstatus
	{
		idle = 1,
		waitNetwork = 2,
		LinkErr = 3,
		PINerr = 4,
		SIMerr = 5,
		noReply = 6,
		PUKerr = 7,
		invalidFrame = 8,
		couldNotPwrModem = 9,
		connecting,
		transfering
	};

private:
	static cLED* _instance;
	cLED();

	cyg_bool mStarted;

	eLEDstatus mStatus;
	cyg_mutex_t mStatMutex;
	cyg_uint32 mLED_h;
	cyg_uint32 mLED_l;

	wdKicker* mWatchDog;

	void setGreen();
	void setRed();

public:

	static void init();
	static cLED* get();

	void start();

	void animate();
	void toggleLED();

	void indicate(eLEDstatus stat);

	eLEDstatus getStatus();

	virtual ~cLED();
};

#endif /* LED_H_ */
