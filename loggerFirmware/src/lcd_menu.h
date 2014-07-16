#ifndef LCD_MENU_H_
#define LCD_MENU_H_
#include <cyg/kernel/kapi.h>

#include "picaxe_lcd.h"

class cLCDmenu
{
	cyg_bool mOpened;

protected:
	const char* mHeading;
	cPICAXEserialLCD* mLCD;

public:
	cLCDmenu(cPICAXEserialLCD* lcd, const char* heading);

	virtual void open() = 0;
	cyg_bool isOpen(){ return mOpened; };

	const char* getHeading(){ return mHeading; };

	virtual void up() = 0;
	virtual void down() = 0;
	virtual void enter() = 0;
	virtual void cancel() = 0;

	virtual void returnParentMenu(){};

	virtual ~cLCDmenu();
};

#endif /* LCD_MENU_H_ */
