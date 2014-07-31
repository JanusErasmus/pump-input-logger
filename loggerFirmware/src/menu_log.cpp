#include <cyg/kernel/diag.h>

#include "menu_log.h"
#include "menu_pump_interval.h"

cLogMenu::cLogMenu(cPICAXEserialLCD* lcd, cLCDmenu * parent) : cLCDmenu(lcd, "LOGS", parent)
{
	mMenuCnt = 1;
	mCursurPos = 2;
}

void cLogMenu::open()
{
	mLCD->clear();
	mLCD->println(1,mHeading);

	mCursurPos = 2;
	mSubMenu = 0;

	//list all the sub menus

	mLCD->println(2, "- Intervals");
	mLCD->println(3, "- Day summaries");
	mLCD->println(4, "- Remove");

	mLCD->showCursor(mCursurPos,0);
}

void cLogMenu::handleEnter()
{
	if((mCursurPos - 1) > mMenuCnt)
		return;

	mLCD->hideCursor();

	switch(mCursurPos - 2)
	{
	case 0:
		mSubMenu = new cPumpIntervalMenu(mLCD, this);
		break;

	default:
		break;
	}

	if(mSubMenu)
		mSubMenu->open();
}

void cLogMenu::handleCancel()
{
	if(mParent)
	mParent->returnParentMenu();
}

void cLogMenu::handleUp()
{
	if(--mCursurPos == 1)
		mCursurPos = 2;

	mLCD->setCursor(mCursurPos,0);
}

void cLogMenu::handleDown()
{
	if(++mCursurPos > 4)
		mCursurPos = 2;

	mLCD->setCursor(mCursurPos,0);
}

cLogMenu::~cLogMenu()
{
}

