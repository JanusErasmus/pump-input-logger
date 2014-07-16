#ifndef LOG_MENU_H_
#define LOG_MENU_H_
#include "lcd_menu.h"

class cLogMenu : public cLCDmenu
{
	cLCDmenu* mParent;

	void showLog();

public:
	cLogMenu(cPICAXEserialLCD* lcd, cLCDmenu* parent = 0);

	void open();

	void enter();
	void cancel();
	void up();
	void down();

	virtual ~cLogMenu();
};

#endif /* LOG_MENU_H_ */