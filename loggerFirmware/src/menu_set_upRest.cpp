#include <cyg/kernel/diag.h>

#include "menu_set_upRest.h"
#include "menu_get_number.h"
#include "nvm.h"

cSetUpRestMenu::cSetUpRestMenu(cLineDisplay * lcd, cLCDmenu* parent) : cLCDmenu(lcd, "SET UP/REST TIMES", parent)
{
	mCursurPos = 3;
}

void cSetUpRestMenu::open()
{
	mLCD->clear();
	mLCD->println(1,mHeading);
	mLCD->println(2,"UP:%02dm      REST:%02dm", cNVM::get()->getPumpUpTime(), cNVM::get()->getPumpRestTime());
	mLCD->println(3,"- Set UP time");
	mLCD->println(4,"- Set REST time");

	mLCD->showCursor(mCursurPos,0);
}

void cSetUpRestMenu::handleEnter()
{
	switch(mCursurPos)
	{
	case 3:
		mSubMenu = new cGetNumberMenu("UP time [m]", cNVM::get()->getPumpUpTime(), mLCD, this);
		mLCD->hideCursor();
		mSubMenu->open();
		break;
	case 4:
		mSubMenu = new cGetNumberMenu("REST time [m]", cNVM::get()->getPumpRestTime(), mLCD, this);
		mLCD->hideCursor();
		mSubMenu->open();
		break;
	default:
		break;
	}

}

void cSetUpRestMenu::handleCancel()
{
	if(mParent)
		mParent->returnParentMenu();
}

void cSetUpRestMenu::handleUp()
{
	if(--mCursurPos == 2)
			mCursurPos = 3;

		mLCD->setCursor(mCursurPos,0);
}

void cSetUpRestMenu::handleDown()
{
	if(++mCursurPos > 4)
			mCursurPos = 3;

		mLCD->setCursor(mCursurPos,0);
}

void cSetUpRestMenu::returnParentMenu()
{
	if(mSubMenu)
	{
		cyg_uint8 number = ((cGetNumberMenu*)mSubMenu)->getNumber();

		if(number != 0xFF)
		{
			switch(mCursurPos)
			{
			case 3:
				diag_printf("Set UP time %d\n", number);
				cNVM::get()->setPumpUpTime(number);
				break;
			case 4:
				diag_printf("Set REST time %d\n", number);
				cNVM::get()->setPumpRestTime(number);
				break;
			default:
				break;
			}
		}

		delete mSubMenu;
		mSubMenu = 0;
	}

	open();
}

cSetUpRestMenu::~cSetUpRestMenu()
{
}

