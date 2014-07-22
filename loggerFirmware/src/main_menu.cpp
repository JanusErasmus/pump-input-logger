#include <cyg/kernel/diag.h>

#include "main_menu.h"
#include "log_menu.h"
#include "set_time_menu.h"

cMainMenu::cMainMenu(cPICAXEserialLCD* lcd) : cLCDmenu(lcd, "Main MENU")
{
	mSubMenus[0] = new cLogMenu(lcd, this);
	mSubMenus[1] = new cSetTimeMenu(lcd, this);

	mMenuCnt = 2;
	mCursurPos = 2;
}

void cMainMenu::open()
{
	mLCD->clear();
	mLCD->println(1,mHeading);

	//list all the sub menus
	for(cyg_uint8 k = 0; k < mMenuCnt; k++)
	{
		//there is space for 3 items on startup
		if(k > 3)
			break;

		mLCD->println(k + 2, "- %s", mSubMenus[k]->getHeading());
	}

	mLCD->showCursor(mCursurPos,0);
}

void cMainMenu::handleEnter()
{
	diag_printf("Main: enter %d\n", mCursurPos);
	if((mCursurPos - 1) > mMenuCnt)
		return;

	mLCD->hideCursor();
	mSubMenu = mSubMenus[mCursurPos - 2];
	mSubMenu->open();
}

void cMainMenu::handleCancel()
{
	diag_printf("Main: cancel\n");
}

void cMainMenu::handleUp()
{
	diag_printf("Main: up\n");
	if(--mCursurPos == 1)
		mCursurPos = 2;

	mLCD->setCursor(mCursurPos,0);
}

void cMainMenu::handleDown()
{
	diag_printf("Main: down\n");
	if(++mCursurPos > 4)
		mCursurPos = 2;

	mLCD->setCursor(mCursurPos,0);
}

void cMainMenu::returnParentMenu()
{
	mSubMenu = 0;
	open();
}

cMainMenu::~cMainMenu()
{
}

