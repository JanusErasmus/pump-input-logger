#ifndef STANDBY_MENU_H_
#define STANDBY_MENU_H_
#include <time.h>

#include "menu_lcd.h"

class cStandbyMenu : public cLCDmenu
{
	cyg_bool mPumpState;
	cyg_bool mTankLevel;
	cyg_bool mInFrameFlag;
	cyg_bool mRestingFlag;
	time_t mTimeLeft;

	void showStatus();

	void printTimeLeft(char * string, time_t left);

public:
	cStandbyMenu(cPICAXEserialLCD* lcd, cLCDmenu* parent = 0, cyg_uint8 state = 0, cyg_uint8 level = 0, cyg_bool inFrameFlag = 0, time_t timeLeft = 0);

	void setTankLevel(cyg_bool state);
	void setPumpState(cyg_bool state);
	void setInFrameState(cyg_bool state);
	void setRestingState(cyg_bool state);
	void setTimeLeft(time_t left);

	void open();

	void handleEnter();
	void handleCancel();
	void handleUp();
	void handleDown();

	virtual ~cStandbyMenu();
};

#endif /* STANDBY_MENU_H_ */
