#ifndef STANDBY_MENU_H_
#define STANDBY_MENU_H_

#include "menu_lcd.h"

class cStandbyMenu : public cLCDmenu
{
	cyg_bool mPumpState;

public:
	cStandbyMenu(cPICAXEserialLCD* lcd, cLCDmenu* parent = 0);

	void setPumpState(cyg_bool state);

	void open();

	void handleEnter();
	void handleCancel();
	void handleUp();
	void handleDown();

	virtual ~cStandbyMenu();
};

#endif /* STANDBY_MENU_H_ */
