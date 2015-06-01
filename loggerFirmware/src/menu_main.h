#ifndef Main_MENU_H_
#define Main_MENU_H_
#include "menu_lcd.h"

class cMainMenu : public cLCDmenu
{
	cyg_uint8 mMenuCnt;
	cyg_uint8 mCursurPos;

public:
	cMainMenu(cLineDisplay * lcd, cLCDmenu * parent);

	void open();

	void handleEnter();
	void handleCancel();
	void handleUp();
	void handleDown();

	virtual ~cMainMenu();
};

#endif /* Main_MENU_H_ */
