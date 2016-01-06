#ifndef LOG_MENU_H_
#define LOG_MENU_H_
#include "menu_lcd.h"

class cLogMenu : public cLCDmenu
{
	cyg_uint8 mMenuCnt;
	cyg_uint8 mCursurPos;

public:
	cLogMenu(cPICAXEserialLCD* lcd, cLCDmenu * parent);

	void open();

	void handleEnter();
	void handleCancel();
	void handleUp();
	void handleDown();

	virtual ~cLogMenu();
};

#endif /* LOG_MENU_H_ */
