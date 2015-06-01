#include <cyg/kernel/diag.h>
#include <stdio.h>

#include "menu_pump_interval.h"
#include "log.h"

cPumpIntervalMenu::cPumpIntervalMenu(cLineDisplay * lcd, cLCDmenu* parent) : cLCDmenu(lcd, "", parent)
{
	mLogIdx = 0;
}

void cPumpIntervalMenu::open()
{
	mLCD->clear();
	cLog::get()->reset();
	showLog();
}

void cPumpIntervalMenu::handleCancel()
{
	if(mParent)
		mParent->returnParentMenu();
}

void cPumpIntervalMenu::handleDown()
{
	showLog();
}

void cPumpIntervalMenu::showLog()
{
	time_t on,off,duration;

	mLCD->clear();


	if(cLog::get()->getNextOnDuration((cyg_uint8)5, duration, on, off))
	{
		struct tm*  info;

		info = localtime(&on);
		char buffer[20];
		strftime(buffer,20,"%a  %d-%m-%Y", info);

		mLCD->println(1,"#%02d %s", mLogIdx++, buffer);

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
		mLogIdx = 0;
		mLCD->println(3,"END of logs");
	}
}

cPumpIntervalMenu::~cPumpIntervalMenu()
{
}

