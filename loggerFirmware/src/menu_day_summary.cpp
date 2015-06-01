#include <cyg/kernel/diag.h>
#include <stdio.h>

#include "menu_day_summary.h"
#include "log.h"

cPumpDaySummaryMenu::cPumpDaySummaryMenu(cLineDisplay * lcd, cLCDmenu* parent) : cLCDmenu(lcd, "", parent)
{
	mLogIdx = 0;
}

void cPumpDaySummaryMenu::open()
{
	mLCD->clear();
	cLog::get()->reset();
	showLog();
}

void cPumpDaySummaryMenu::handleCancel()
{
	if(mParent)
		mParent->returnParentMenu();
}

void cPumpDaySummaryMenu::handleDown()
{
	showLog();
}

void cPumpDaySummaryMenu::showLog()
{
	time_t on, duration;

	mLCD->clear();


	if(cLog::get()->getNextDayOnDuration((cyg_uint8)5, duration, on))
	{
		struct tm*  info;

		info = localtime(&on);
		char buffer[20];
		strftime(buffer,20,"%A", info);
		mLCD->println(1,"#%02d %s", mLogIdx++, buffer);

		strftime(buffer,20,"%d-%m-%Y", info);
		mLCD->println(2,"    %s", buffer);

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

cPumpDaySummaryMenu::~cPumpDaySummaryMenu()
{
}

