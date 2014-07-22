#include <cyg/kernel/diag.h>

#include "set_time_menu.h"
#include "get_number_menu.h"

cSetTimeMenu::cSetTimeMenu(cPICAXEserialLCD* lcd, cLCDmenu* parent) : cLCDmenu(lcd, "SET TIME", parent)
{
	mCursurPos = 3;
}

void cSetTimeMenu::open()
{
	mLCD->clear();
	mLCD->println(1,mHeading);

	mLCD->println(3,"- Set HOURS");
	mLCD->println(4,"- Set MINUTES");

	mLCD->showCursor(mCursurPos,0);
}

void cSetTimeMenu::handleEnter()
{
	switch(mCursurPos)
	{
	case 3:
		mSubMenu = new cGetNumberMenu("HOUR", 12, mLCD, this);
		mLCD->hideCursor();
		mSubMenu->open();
		break;
	case 4:
		mSubMenu = new cGetNumberMenu("MINUTES", 30, mLCD, this);
		mLCD->hideCursor();
		mSubMenu->open();
		break;
	default:
		break;
	}

}

void cSetTimeMenu::handleCancel()
{
	if(mParent)
		mParent->returnParentMenu();
}

void cSetTimeMenu::handleUp()
{
	if(--mCursurPos == 2)
			mCursurPos = 3;

		mLCD->setCursor(mCursurPos,0);
}

void cSetTimeMenu::handleDown()
{
	if(++mCursurPos > 4)
			mCursurPos = 3;

		mLCD->setCursor(mCursurPos,0);
}

void cSetTimeMenu::returnParentMenu()
{
	if(mSubMenu)
	{
		cyg_uint8 number = ((cGetNumberMenu*)mSubMenu)->getNumber();
		if(number != 0xFF)
		{
			switch(mCursurPos)
			{
			case 3:
				diag_printf("Set HOURS %d\n", number);
				break;
			case 4:
				diag_printf("Set Minutes %d\n", number);
				break;
			default:
				break;
			}
		}

		delete mSubMenu;
	}

	mSubMenu = 0;
	open();
}

cSetTimeMenu::~cSetTimeMenu()
{
}

