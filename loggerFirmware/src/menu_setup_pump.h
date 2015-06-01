#ifndef SETUP_PUMP_MENU_H_
#define SETUP_PUMP_MENU_H_
#include "menu_lcd.h"

class cSetupPumpMenu : public cLCDmenu
{
	cyg_uint8 mMenuCnt;
	cyg_uint8 mCursurPos;

public:
	cSetupPumpMenu(cLineDisplay * lcd, cLCDmenu * parent);

	void open();

	void handleEnter();
	void handleCancel();
	void handleUp();
	void handleDown();

	virtual ~cSetupPumpMenu();
};

#endif /* SETUP_PUMP_MENU_H_ */
