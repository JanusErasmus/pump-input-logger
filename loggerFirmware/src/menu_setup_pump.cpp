#include <cyg/kernel/diag.h>

#include "menu_setup_pump.h"
#include "menu_set_upRest.h"
#include "menu_set_frame.h"

cSetupPumpMenu::cSetupPumpMenu(cPICAXEserialLCD* lcd, cLCDmenu * parent) : cLCDmenu(lcd, "SETUP PUMP", parent)
{
	mMenuCnt = 2;
	mCursurPos = 2;
}

void cSetupPumpMenu::open()
{
	mLCD->clear();
	mLCD->println(1,mHeading);

	mCursurPos = 2;
	mSubMenu = 0;

	//list all the sub menus

	mLCD->println(2, "- SET UP/REST TIMES");
	mLCD->println(3, "- SET PUMP FRAME");

	mLCD->showCursor(mCursurPos,0);
}

void cSetupPumpMenu::handleEnter()
{
	if(mCursurPos > (mMenuCnt + 1))
		return;

	mLCD->hideCursor();

	switch(mCursurPos)
	{
	case 2:
		mSubMenu = new cSetUpRestMenu(mLCD, this);
		break;
	case 3:
		mSubMenu = new cSetFrameMenu(mLCD, this);
		break;

	default:
		break;
	}

	if(mSubMenu)
		mSubMenu->open();
}

void cSetupPumpMenu::handleCancel()
{
	if(mParent)
	mParent->returnParentMenu();
}

void cSetupPumpMenu::handleUp()
{
	if(--mCursurPos == 1)
		mCursurPos = mMenuCnt + 1;

	mLCD->setCursor(mCursurPos,0);
}

void cSetupPumpMenu::handleDown()
{
	if(++mCursurPos > 4)
		mCursurPos = 2;

	mLCD->setCursor(mCursurPos,0);
}

cSetupPumpMenu::~cSetupPumpMenu()
{
}

