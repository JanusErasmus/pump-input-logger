#include <cyg/kernel/diag.h>

#include "main_menu.h"
#include "log_menu.h"

cMainMenu::cMainMenu(cPICAXEserialLCD* lcd) : cLCDmenu(lcd, "Main MENU")
{
	mSubMenus[0] = new cLogMenu(lcd, this);

	mMenuCnt = 1;
	mCursurPos = 2;
	mOpenMenu = 0xFF;
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

	mLCD->showCursor(2,0);
}

void cMainMenu::enter()
{
	if(mOpenMenu == 0xFF)
	{
		diag_printf("Main: enter %d\n", mCursurPos);
		if((mCursurPos - 1) > mMenuCnt)
			return;

		mLCD->hideCursor();
		mOpenMenu = mCursurPos - 2;
		mSubMenus[mOpenMenu]->open();

		return;
	}

	if(mOpenMenu > mMenuCnt)
			return;

	mSubMenus[mOpenMenu]->enter();
}

void cMainMenu::cancel()
{
	if(mOpenMenu == 0xFF)
	{
		diag_printf("Main: cancel\n");
		return;
	}

	if(mOpenMenu > mMenuCnt)
		return;

	mSubMenus[mOpenMenu]->cancel();
}

void cMainMenu::up()
{
	if(mOpenMenu == 0xFF)
	{
		diag_printf("Main: up\n");
		if(--mCursurPos == 1)
			mCursurPos = 2;

		mLCD->setCursor(mCursurPos,0);
		return;
	}

	if(mOpenMenu > mMenuCnt)
		return;

	mSubMenus[mOpenMenu]->up();

}

void cMainMenu::down()
{
	if(mOpenMenu == 0xFF)
	{
		diag_printf("Main: down\n");
		if(++mCursurPos > 4)
			mCursurPos = 2;

		mLCD->setCursor(mCursurPos,0);
		return;
	}

	if(mOpenMenu > mMenuCnt)
		return;

	mSubMenus[mOpenMenu]->down();
}

void cMainMenu::returnParentMenu()
{
	mOpenMenu = 0xFF;
	open();
}

cMainMenu::~cMainMenu()
{
}

