#ifndef DAY_SUMMARY_MENU_H_
#define DAY_SUMMARY_MENU_H_
#include "menu_lcd.h"

class cPumpDaySummaryMenu : public cLCDmenu
{
	cyg_uint8 mLogIdx;
	void showLog();

public:
	cPumpDaySummaryMenu(cPICAXEserialLCD* lcd, cLCDmenu* parent = 0);

	void open();

	void handleCancel();
	void handleDown();

	virtual ~cPumpDaySummaryMenu();
};

#endif /* DAY_SUMMARY_MENU_H_ */
