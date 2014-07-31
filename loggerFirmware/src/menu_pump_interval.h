#ifndef INTERVAL_MENU_H_
#define INTERVAL_MENU_H_
#include "menu_lcd.h"

class cPumpIntervalMenu : public cLCDmenu
{
	void showLog();

public:
	cPumpIntervalMenu(cPICAXEserialLCD* lcd, cLCDmenu* parent = 0);

	void open();

	void handleEnter();
	void handleCancel();
	void handleUp();
	void handleDown();

	virtual ~cPumpIntervalMenu();
};

#endif /* INTERVAL_MENU_H_ */
