#include "menu_lcd.h"

cLCDmenu::cLCDmenu(cLineDisplay * lcd, const char* heading, cLCDmenu * parent) :
	mSubMenu(0),
	mHeading(heading),
	mLCD(lcd),
	mParent(parent)
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

void cLCDmenu::returnParentMenu()
{
	if(mSubMenu)
		delete mSubMenu;

	mSubMenu = 0;
	open();
}

cLCDmenu::~cLCDmenu()
{
	if(mSubMenu)
		delete mSubMenu;
}

