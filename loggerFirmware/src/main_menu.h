#ifndef Main_MENU_H_
#define Main_MENU_H_
#include "lcd_menu.h"

class cMainMenu : public cLCDmenu
{

	cLCDmenu* mSubMenus[1];
	cyg_uint8 mMenuCnt;
	cyg_uint8 mCursurPos;
	cyg_uint8 mOpenMenu;

public:
	cMainMenu(cPICAXEserialLCD* lcd);

	void open();

	void enter();
	void cancel();
	void up();
	void down();

	void returnParentMenu();

	virtual ~cMainMenu();
};

#endif /* Main_MENU_H_ */
