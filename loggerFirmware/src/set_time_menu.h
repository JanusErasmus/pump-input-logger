#ifndef SET_TIME_MENU_H_
#define SET_TIME_MENU_H_

#include "lcd_menu.h"

class cSetTimeMenu : public cLCDmenu
{
	cyg_uint8 mCursurPos;
	cyg_uint8 mHours;
	cyg_uint8 mMinutes;

	void setHours(cyg_uint8 hours);
	void setMinutes(cyg_uint8 min);


public:
	cSetTimeMenu(cPICAXEserialLCD* lcd, cLCDmenu* parent = 0);

	void open();

	void handleEnter();
	void handleCancel();
	void handleUp();
	void handleDown();

	void returnParentMenu();

	virtual ~cSetTimeMenu();
};

#endif /* SET_TIME_MENU_H_ */
