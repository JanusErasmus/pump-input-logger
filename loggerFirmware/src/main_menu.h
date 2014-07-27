#ifndef Main_MENU_H_
#define Main_MENU_H_
#include "lcd_menu.h"

class cMainMenu : public cLCDmenu
{
	cyg_uint8 mMenuCnt;
	cyg_uint8 mCursurPos;

public:
	cMainMenu(cPICAXEserialLCD* lcd, cLCDmenu * parent);

	void open();

	void handleEnter();
	void handleCancel();
	void handleUp();
	void handleDown();

	virtual ~cMainMenu();
};

#endif /* Main_MENU_H_ */
