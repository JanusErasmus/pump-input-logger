#include <cyg/kernel/diag.h>
#include <stdio.h>

#include "menu_standby.h"
#include "MCP_rtc.h"
#include "menu_main.h"

cStandbyMenu::cStandbyMenu(cPICAXEserialLCD* lcd, cLCDmenu* parent) : cLCDmenu(lcd, "PUMP LOGGER", parent)
{
	mPumpState = 0;
}

void cStandbyMenu::setPumpState(cyg_bool state)
{
	mPumpState = state;

	if(!mSubMenu)
	{
		cPICAXEserialLCD::get()->println(3,"PUMP %s",mPumpState?"RUNNING":"STOPPED");
	}
}

void cStandbyMenu::open()
{
	time_t now = cRTC::get()->timeNow();
	struct tm*  info = localtime(&now);

	cPICAXEserialLCD::get()->hideCursor();
	cPICAXEserialLCD::get()->clear();
	cPICAXEserialLCD::get()->println(1, "PUMP LOGGER    %02d:%02d", info->tm_hour, info->tm_min);

	cPICAXEserialLCD::get()->println(3,"PUMP %s",mPumpState?"RUNNING":"STOPPED");
}

void cStandbyMenu::handleEnter()
{
	diag_printf("Standby: enter\n");

	mSubMenu = new cMainMenu(mLCD, this);
	mSubMenu->open();
}

void cStandbyMenu::handleCancel()
{
	diag_printf("Standby: cancel\n");
}

void cStandbyMenu::handleUp()
{
	diag_printf("Standby: up\n");
}

void cStandbyMenu::handleDown()
{
	diag_printf("Standby: down\n");
}

cStandbyMenu::~cStandbyMenu()
{
}

