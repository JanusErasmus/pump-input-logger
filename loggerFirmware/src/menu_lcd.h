#ifndef LCD_MENU_H_
#define LCD_MENU_H_
#include <cyg/kernel/kapi.h>

#include "picaxe_lcd.h"

class cLCDmenu
{

protected:
	cLCDmenu* mSubMenu;
	const char* mHeading;
	cPICAXEserialLCD* mLCD;
	cLCDmenu* mParent;

	virtual void handleUp(){};
	virtual void handleDown(){};
	virtual void handleEnter(){};
	virtual void handleCancel(){};

public:
	cLCDmenu(cPICAXEserialLCD* lcd, const char* heading, cLCDmenu * parent = 0);

	virtual void open() = 0;

	const char* getHeading(){ return mHeading; };

	void up();
	void down();
	void enter();
	void cancel();

	virtual void returnParentMenu();

	virtual ~cLCDmenu();
};

#endif /* LCD_MENU_H_ */
