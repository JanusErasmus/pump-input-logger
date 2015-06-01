#include <cyg/kernel/diag.h>

#include "menu_log_ack.h"
#include "log.h"
#include "MCP_rtc.h"

cLogAckMenu::cLogAckMenu(cLineDisplay * lcd, cLCDmenu* parent) : cLCDmenu(lcd, "REMOVE LOGS", parent)
{
	mCursurPos = 2;
}

void cLogAckMenu::open()
{
	mCursurPos = 2;

	mLCD->clear();
	mLCD->println(1,mHeading);
	mLCD->println(2, "- Older than a day");
	mLCD->println(3, "- Older than a week");
	mLCD->println(4, "- All");


	mLCD->showCursor(mCursurPos,0);
}

void cLogAckMenu::handleEnter()
{
	mLCD->clear();

	time_t now = cRTC::get()->timeNow();
	struct tm *info = localtime(&now);
	info->tm_hour = 0;
	info->tm_min = 0;
	info->tm_sec = 0;

	switch(mCursurPos)
	{
	case 2:
	{
		mLCD->println(2, "Removing Logs");
		mLCD->println(3, "older than a day");

		while(cLog::get()->acknowledge(mktime(info)));
	}
	break;
	case 3:
	{
		mLCD->println(2, "Removing Logs");
		mLCD->println(3, "older than a week");

		info->tm_wday -= 7;

		while(cLog::get()->acknowledge(mktime(info)));

	}
	break;
	case 4:
	{
		mLCD->println(3, "Removing All Logs");
		while(cLog::get()->acknowledge());
	}
	break;

	default:
		break;
	}

	cyg_thread_delay(1000);

	if(mParent)
		mParent->returnParentMenu();
}

void cLogAckMenu::handleCancel()
{
	if(mParent)
		mParent->returnParentMenu();
}

void cLogAckMenu::handleUp()
{
	if(--mCursurPos == 1)
		mCursurPos = 4;

	mLCD->setCursor(mCursurPos,0);
}

void cLogAckMenu::handleDown()
{
	if(++mCursurPos > 4)
		mCursurPos = 2;

	mLCD->setCursor(mCursurPos,0);
}


cLogAckMenu::~cLogAckMenu()
{
}

