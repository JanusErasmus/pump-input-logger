#ifndef LOG_MENU_H_
#define LOG_MENU_H_
#include "lcd_menu.h"

class cLogMenu : public cLCDmenu
{
	void showLog();

public:
	cLogMenu(cPICAXEserialLCD* lcd, cLCDmenu* parent = 0);

	void open();

	void handleEnter();
	void handleCancel();
	void handleUp();
	void handleDown();

	virtual ~cLogMenu();
};

#endif /* LOG_MENU_H_ */
