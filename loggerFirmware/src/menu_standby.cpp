#include <cyg/kernel/diag.h>
#include <stdio.h>

#include "menu_standby.h"
#include "MCP_rtc.h"
#include "menu_main.h"

cStandbyMenu::cStandbyMenu(
		cPICAXEserialLCD* lcd,
		cLCDmenu* parent,
		cyg_uint8 pumpState,
		cyg_uint8 tankLevel,
		cyg_bool inFrameFlag
		) : cLCDmenu(lcd, "PUMP LOGGER", parent)
{
	mPumpState = pumpState;
	mTankLevel = tankLevel;
	mInFrameFlag = inFrameFlag;
	mRestingFlag = false;
}

void cStandbyMenu::showStatus()
{
	cPICAXEserialLCD::get()->println(3,"TANK %s",mTankLevel?"FULL":"LOW ");

	if(mPumpState)
	{
		cPICAXEserialLCD::get()->println        (4,"PUMP ON            ");
	}
	else
	{
		if(mInFrameFlag)
		{
			if(mRestingFlag)
				cPICAXEserialLCD::get()->println(4,"PUMP OFF (RESTING) ");
			else
				cPICAXEserialLCD::get()->println(4,"PUMP OFF           ");
		}
		else
			cPICAXEserialLCD::get()->println    (4,"PUMP OFF (DISABLED)");
	}
}

void cStandbyMenu::setTankLevel(cyg_bool state)
{
	if(mTankLevel == state)
			return;

	mTankLevel = state;

	if(!mSubMenu)
	{
		showStatus();
	}
}

void cStandbyMenu::setPumpState(cyg_bool state)
{
	if(mPumpState == state)
			return;

	mPumpState = state;

	if(!mSubMenu)
	{
		showStatus();
	}
}

void cStandbyMenu::setInFrameState(cyg_bool state)
{
	if(mInFrameFlag == state)
		return;

	mInFrameFlag = state;

	if(!mSubMenu)
	{
		showStatus();
	}
}

void cStandbyMenu::setRestingState(cyg_bool state)
{
	if(mRestingFlag == state)
			return;

	mRestingFlag = state;

		if(!mSubMenu)
		{
			showStatus();
		}
}

void cStandbyMenu::open()
{
	time_t now = cRTC::get()->timeNow();
	struct tm*  info = localtime(&now);

	cPICAXEserialLCD::get()->hideCursor();
	cPICAXEserialLCD::get()->clear();
	cPICAXEserialLCD::get()->println(1, "PUMP LOGGER    %02d:%02d", info->tm_hour, info->tm_min);
	showStatus();
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

