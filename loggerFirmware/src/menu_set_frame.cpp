#include <cyg/kernel/diag.h>

#include "menu_set_frame.h"
#include "menu_get_number.h"
#include "nvm.h"

cSetFrameMenu::cSetFrameMenu(cLineDisplay * lcd, cLCDmenu* parent) : cLCDmenu(lcd, "SET PUMP FRAME", parent)
{
	mCursurPos = 3;
}

void cSetFrameMenu::open()
{
	mLCD->clear();
	mLCD->println(1,mHeading);
	mLCD->println(2,"START:%02dh  END:%02dh", cNVM::get()->getPumpFrameStart(), cNVM::get()->getPumpFrameEnd());
	mLCD->println(3,"- Set START time");
	mLCD->println(4,"- Set END time");

	mLCD->showCursor(mCursurPos,0);
}

void cSetFrameMenu::handleEnter()
{
	switch(mCursurPos)
	{
	case 3:
		mSubMenu = new cGetNumberMenu("START HOUR", cNVM::get()->getPumpFrameStart(), mLCD, this);
		mLCD->hideCursor();
		mSubMenu->open();
		break;
	case 4:
		mSubMenu = new cGetNumberMenu("END HOUR", cNVM::get()->getPumpFrameEnd(), mLCD, this);
		mLCD->hideCursor();
		mSubMenu->open();
		break;
	default:
		break;
	}

}

void cSetFrameMenu::handleCancel()
{
	if(mParent)
		mParent->returnParentMenu();
}

void cSetFrameMenu::handleUp()
{
	if(--mCursurPos == 2)
			mCursurPos = 3;

		mLCD->setCursor(mCursurPos,0);
}

void cSetFrameMenu::handleDown()
{
	if(++mCursurPos > 4)
			mCursurPos = 3;

		mLCD->setCursor(mCursurPos,0);
}

void cSetFrameMenu::returnParentMenu()
{
	if(mSubMenu)
	{
		cyg_uint8 number = ((cGetNumberMenu*)mSubMenu)->getNumber();

		if(number != 0xFF)
		{
			switch(mCursurPos)
			{
			case 3:
				diag_printf("Set START time %d\n", number);
				if(number < 24 && number <= cNVM::get()->getPumpFrameEnd())
					cNVM::get()->setPumpFrameStart(number);
				break;
			case 4:
				diag_printf("Set END time %d\n", number);
				if(number < 24 && number > cNVM::get()->getPumpFrameStart())
					cNVM::get()->setPumpFrameEnd(number);
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

cSetFrameMenu::~cSetFrameMenu()
{
}

