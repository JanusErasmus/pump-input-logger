#ifndef GET_NUMBER_MENU_H_
#define GET_NUMBER_MENU_H_

#include "menu_lcd.h"

class cGetNumberMenu: public cLCDmenu
{
	cyg_uint8 mNumber;

public:
	cGetNumberMenu(const char* numberName, cyg_uint8 number, cLineDisplay * lcd, cLCDmenu* parent);

	void open();

	void handleEnter();
	void handleCancel();
	void handleUp();
	void handleDown();

	cyg_uint8 getNumber(){ return mNumber; };

	virtual ~cGetNumberMenu();
};

#endif /* GET_NUMBER_MENU_H_ */
