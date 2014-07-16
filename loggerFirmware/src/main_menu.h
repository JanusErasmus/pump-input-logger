#ifndef Main_MENU_H_
#define Main_MENU_H_
#include "lcd_menu.h"

class cMainMenu : public cLCDmenu
{

	cLCDmenu* mSubMenus[1];
	cyg_uint8 mMenuCnt;
	cyg_uint8 mCursurPos;

public:
	cMainMenu(cPICAXEserialLCD* lcd);

	void open();

	void handleEnter();
	void handleCancel();
	void handleUp();
	void handleDown();

	void returnParentMenu();

	virtual ~cMainMenu();
};

#endif /* Main_MENU_H_ */
