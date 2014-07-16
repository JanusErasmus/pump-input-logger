#include "lcd_menu.h"

cLCDmenu::cLCDmenu(cPICAXEserialLCD* lcd, const char* heading) :
	mOpened(false),
	mSubMenu(0),
	mHeading(heading),
	mLCD(lcd)
{
}

void cLCDmenu::up()
{
	if(mSubMenu)
	{
		mSubMenu->up();
		return;
	}

	handleUp();
}

void cLCDmenu::down()
{
	if(mSubMenu)
	{
		mSubMenu->down();
		return;
	}

	handleDown();
}

void cLCDmenu::enter()
{
	if(mSubMenu)
	{
		mSubMenu->enter();
		return;
	}

	handleEnter();
}

void cLCDmenu::cancel()
{
	if(mSubMenu)
	{
		mSubMenu->cancel();
		return;
	}

	handleCancel();
}

cLCDmenu::~cLCDmenu()
{
}

