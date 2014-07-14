#ifndef LED_H_
#define LED_H_
#include <cyg/kernel/kapi.h>
#include "definitions.h"
#include "watchdog.h"

class cLED
{
public :
	struct ledPins_s
	{
		cyg_uint8 greenPin;
		cyg_uint8 redPin;
	};

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
		transfering,
		alarm,
		walkOut
	};

private:
	enum eLEDcolor
	{
		red,
		green,
		off
	};

	cLED(ledPins_s* pinNumbers, cyg_uint8 pinCount);
	static cLED* __instance;
	cyg_mutex_t mStatMutex;
	eLEDstatus mStatus;

	cyg_bool mLEDenabled;
	cyg_uint8 mLEDCnt;
	ledPins_s* mLEDList;

	cyg_uint8 mLEDStack[LED_STACK_SIZE];
	cyg_thread mLEDThread;
	cyg_handle_t mLEDThreadHandle;
	static void led_thread(cyg_addrword_t args);

	wdKicker* mWatchDog;

	void setupPorts(ledPins_s* ports, cyg_uint8 count);

	void setLED(cyg_uint8, eLEDcolor);

	void animate();
	void animateWalkout();
	void animateTest();
	void animateIdle();
	void animateFlicker(eLEDcolor c);
	void animateError(cyg_uint8 error);


public:
	static void init(ledPins_s* pinNumbers, cyg_uint8 pinCount);
	static cLED* get();

	void disable(){ mLEDenabled = false; };
	void enable(){ mLEDenabled = true; };
	void indicate(eLEDstatus stat);

	void showIO(cyg_uint8, bool);
};

#endif /* LED_H_ */
