#include <cyg/kernel/diag.h>
#include <stdio.h>

#include "log_menu.h"
#include "log_ack_menu.h"
#include "log.h"

cLogMenu::cLogMenu(cPICAXEserialLCD* lcd, cLCDmenu* parent) : cLCDmenu(lcd, "", parent)
{
}

void cLogMenu::open()
{
	mLCD->clear();
	cLog::get()->reset();
	showLog();
}

void cLogMenu::handleEnter()
{
	diag_printf("LOG: enter\n");
	mSubMenu = new cLogAckMenu(mLCD, this);
	mSubMenu->open();
}

void cLogMenu::handleCancel()
{
	diag_printf("LOG: cancel\n");

	if(mParent)
		mParent->returnParentMenu();
}

void cLogMenu::handleUp()
{
	diag_printf("LOG: up\n");
}

void cLogMenu::handleDown()
{
	diag_printf("LOG: down\n");
	showLog();
}

void cLogMenu::showLog()
{
	time_t on,off,duration;

	mLCD->clear();


	if(cLog::get()->getNextOnDuration(5, duration, on, off))
	{
		struct tm*  info;

		info = localtime(&on);
		mLCD->println(1,"LOG: %02d-%02d-%d", info->tm_mday, info->tm_mon, info->tm_year + 1900);

		mLCD->println(2,"ON : %02d:%02d:%02d", info->tm_hour, info->tm_min, info->tm_sec);

		info = localtime(&off);
		mLCD->println(3,"OFF: %02d:%02d:%02d", info->tm_hour, info->tm_min, info->tm_sec);

		info = localtime(&duration);
		char durationString[16];
		durationString[0] = ' ';
		durationString[1] = ' ';
		durationString[2] = 0x7E;
		if(info->tm_hour)
		{
			sprintf(&durationString[3],"  %dh%dm%ds", info->tm_hour, info->tm_min, info->tm_sec);
		}
		else if(info->tm_min)
		{
			sprintf(&durationString[3],"  %dm%ds", info->tm_min, info->tm_sec);
		}
		else
		{
			sprintf(&durationString[3],"  %ds", info->tm_sec);
		}
		mLCD->println(4,durationString);
	}
	else
	{
		mLCD->println(3,"END of logs");
	}


}

cLogMenu::~cLogMenu()
{
}

