#include <cyg/kernel/diag.h>

#include "menu_set_time.h"
#include "menu_get_number.h"
#include "MCP_rtc.h"

cSetTimeMenu::cSetTimeMenu(cPICAXEserialLCD* lcd, cLCDmenu* parent) : cLCDmenu(lcd, "SET TIME", parent)
{
	mCursurPos = 3;
	mHours = 0;
	mMinutes = 0;
}

void cSetTimeMenu::open()
{
	time_t now = cRTC::get()->timeNow();
	struct tm*  info = localtime(&now);
	mHours = info->tm_hour;
	mMinutes = info->tm_min;

	mLCD->clear();
	mLCD->println(1,mHeading);
	mLCD->println(2,"   %02d:%02d", mHours, mMinutes);
	mLCD->println(3,"- Set HOURS");
	mLCD->println(4,"- Set MINUTES");

	mLCD->showCursor(mCursurPos,0);
}

void cSetTimeMenu::handleEnter()
{
	switch(mCursurPos)
	{
	case 3:
		mSubMenu = new cGetNumberMenu("HOUR", mHours, mLCD, this);
		mLCD->hideCursor();
		mSubMenu->open();
		break;
	case 4:
		mSubMenu = new cGetNumberMenu("MINUTES", mMinutes, mLCD, this);
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
				setHours(number);
				break;
			case 4:
				diag_printf("Set Minutes %d\n", number);
				setMinutes(number);
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

void cSetTimeMenu::setHours(cyg_uint8 hours)
{
	time_t now = cRTC::get()->timeNow();
	struct tm*  info = localtime(&now);

	info->tm_hour = hours;
	info->tm_sec = 0;

	time_t then = 0;
	then = mktime(info);
	cRTC::get()->setTime(&then);
}

void cSetTimeMenu::setMinutes(cyg_uint8 min)
{
	time_t now = cRTC::get()->timeNow();
	struct tm*  info = localtime(&now);

	info->tm_min = min;
	info->tm_sec = 0;

	time_t then = 0;
	then = mktime(info);
	cRTC::get()->setTime(&then);
}

cSetTimeMenu::~cSetTimeMenu()
{
}

