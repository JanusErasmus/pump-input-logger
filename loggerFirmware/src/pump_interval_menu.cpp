#include <cyg/kernel/diag.h>
#include <stdio.h>

#include "pump_interval_menu.h"
#include "log_ack_menu.h"
#include "log.h"

cPumpIntervalMenu::cPumpIntervalMenu(cPICAXEserialLCD* lcd, cLCDmenu* parent) : cLCDmenu(lcd, "", parent)
{
}

void cPumpIntervalMenu::open()
{
	mLCD->clear();
	cLog::get()->reset();
	showLog();
}

void cPumpIntervalMenu::handleEnter()
{
	mSubMenu = new cLogAckMenu(mLCD, this);
	mSubMenu->open();
}

void cPumpIntervalMenu::handleCancel()
{
	if(mParent)
		mParent->returnParentMenu();
}

void cPumpIntervalMenu::handleUp()
{
	diag_printf("LOG: up\n");
}

void cPumpIntervalMenu::handleDown()
{
	showLog();
}

void cPumpIntervalMenu::showLog()
{
	static cyg_uint8 logIdx = 0;
	time_t on,off,duration;

	mLCD->clear();


	if(cLog::get()->getNextOnDuration((cyg_uint8)5, duration, on, off))
	{
		struct tm*  info;

		info = localtime(&on);
		char buffer[20];
		strftime(buffer,20,"%a  %d-%m-%Y", info);

		mLCD->println(1,"#%02d %s", logIdx++, buffer);

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
		logIdx = 0;
		mLCD->println(3,"END of logs");
	}
}

cPumpIntervalMenu::~cPumpIntervalMenu()
{
}

