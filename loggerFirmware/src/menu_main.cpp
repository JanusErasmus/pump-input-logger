#include <cyg/kernel/diag.h>

#include "menu_main.h"
#include "menu_log.h"
#include "menu_set_time.h"

cMainMenu::cMainMenu(cPICAXEserialLCD* lcd, cLCDmenu * parent) : cLCDmenu(lcd, "Main MENU", parent)
{
	mMenuCnt = 2;
	mCursurPos = 2;
}

void cMainMenu::open()
{
	mLCD->clear();
	mLCD->println(1,mHeading);

	mCursurPos = 2;
	mSubMenu = 0;

	//list all the sub menus

	mLCD->println(2, "- LOGS");
	mLCD->println(3, "- SET TIME");

	mLCD->showCursor(mCursurPos,0);
}

void cMainMenu::handleEnter()
{
	if((mCursurPos - 1) > mMenuCnt)
		return;

	mLCD->hideCursor();

	switch(mCursurPos - 2)
	{
	case 0:
		mSubMenu = new cLogMenu(mLCD, this);
		break;
	case 1:
		mSubMenu = new cSetTimeMenu(mLCD, this);
		break;

	default:
		break;
	}

	if(mSubMenu)
		mSubMenu->open();
}

void cMainMenu::handleCancel()
{
	if(mParent)
	mParent->returnParentMenu();
}

void cMainMenu::handleUp()
{
	if(--mCursurPos == 1)
		mCursurPos = 2;

	mLCD->setCursor(mCursurPos,0);
}

void cMainMenu::handleDown()
{
	if(++mCursurPos > 4)
		mCursurPos = 2;

	mLCD->setCursor(mCursurPos,0);
}

cMainMenu::~cMainMenu()
{
}

