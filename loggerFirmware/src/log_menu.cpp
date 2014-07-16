#include <cyg/kernel/diag.h>

#include "log_menu.h"
#include "log.h"

cLogMenu::cLogMenu(cPICAXEserialLCD* lcd, cLCDmenu* parent) : cLCDmenu(lcd, "ON/OFF LOGS"), mParent(parent)
{
}

void cLogMenu::open()
{
	mLCD->clear();
	mLCD->println(1,mHeading);
	showLog();
}

void cLogMenu::handleEnter()
{
	diag_printf("LOG: enter\n");
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
	cLog::get()->readPrev();
	showLog();
}

void cLogMenu::handleDown()
{
	diag_printf("LOG: down\n");
	cLog::get()->readNext();
	showLog();
}

void cLogMenu::showLog()
{
	cyg_bool stat = true;
	cEvent e;

	if(!cLog::get()->readEvent(&e))
	{
		cLog::get()->reset();
		stat = cLog::get()->readEvent(&e);
	}

	mLCD->println(2,"PUMP CHANGED:");

	if(stat)
	{
		if(e.getType() == cEvent::EVENT_TEMP)
		{
			//printf("Value: %.1f\t", mData.mTemp);
		}

		if(e.getType() == cEvent::EVENT_INPUT)
		{
			mLCD->println(3, "Port %d %s", e.getPort(), e.getState()?"ON  ":"OFF");
		}

		//printf("Value: %.1f\t", mData.mTemp);
		cyg_uint32 evtTime = e.getTimeStamp();
		mLCD->println(4, ctime((time_t*)&evtTime));
	}
	else
	{
		mLCD->println(2,"EMPTY");
	}
}

cLogMenu::~cLogMenu()
{
}

