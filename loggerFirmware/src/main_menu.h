#ifndef Main_MENU_H_
#define Main_MENU_H_
#include "lcd_menu.h"

class cMainCancelSignal
{
public:
	virtual void mainCanceled() = 0;
};

class cMainMenu : public cLCDmenu
{
	cLCDmenu* mSubMenus[5];
	cyg_uint8 mMenuCnt;
	cyg_uint8 mCursurPos;

	cMainCancelSignal * mCancelMain;

public:
	cMainMenu(cPICAXEserialLCD* lcd, cMainCancelSignal * cancel);

	void open();

	void handleEnter();
	void handleCancel();
	void handleUp();
	void handleDown();

	void returnParentMenu();

	virtual ~cMainMenu();
};

#endif /* Main_MENU_H_ */
