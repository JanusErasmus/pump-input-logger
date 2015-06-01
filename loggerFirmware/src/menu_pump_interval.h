#ifndef INTERVAL_MENU_H_
#define INTERVAL_MENU_H_
#include "menu_lcd.h"

class cPumpIntervalMenu : public cLCDmenu
{
	cyg_uint8 mLogIdx;
	void showLog();

public:
	cPumpIntervalMenu(cLineDisplay * lcd, cLCDmenu* parent = 0);

	void open();

	void handleCancel();
	void handleDown();

	virtual ~cPumpIntervalMenu();
};

#endif /* INTERVAL_MENU_H_ */
