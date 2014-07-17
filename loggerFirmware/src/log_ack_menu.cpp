#include <cyg/kernel/diag.h>

#include "log_ack_menu.h"
#include "log.h"

cLogAckMenu::cLogAckMenu(cPICAXEserialLCD* lcd, cLCDmenu* parent) : cLCDmenu(lcd, "REMOVE LOG?"), mParent(parent)
{

}

void cLogAckMenu::open()
{
	mLCD->clear();
	mLCD->println(1,mHeading);
	mLCD->println(3, "YES <ENTER>");
	mLCD->println(4, "NO  <CANCEL>");
}

void cLogAckMenu::handleEnter()
{
	diag_printf("LOGack: enter\n");
	cLog::get()->acknowledge();
	cLog::get()->reset();

	if(mParent)
		mParent->returnParentMenu();
}

void cLogAckMenu::handleCancel()
{
	diag_printf("LOGack: cancel\n");

	if(mParent)
		mParent->returnParentMenu();
}


cLogAckMenu::~cLogAckMenu()
{

}

