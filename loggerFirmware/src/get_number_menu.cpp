#include "get_number_menu.h"

cGetNumberMenu::cGetNumberMenu(const char* numberName, cyg_uint8 number, cPICAXEserialLCD* lcd, cLCDmenu* parent) :
	cLCDmenu(lcd, numberName, parent),
	mNumber(number)
{
}

void cGetNumberMenu::open()
{
	mLCD->clear();
	mLCD->println(1,"Set: %s", mHeading);

	mLCD->println(3," [%2d]", mNumber);

}

void cGetNumberMenu::handleEnter()
{
	if(mParent)
			mParent->returnParentMenu();
}

void cGetNumberMenu::handleCancel()
{
	mNumber = 0xFF;
	if(mParent)
			mParent->returnParentMenu();
}

void cGetNumberMenu::handleUp()
{
	if(++mNumber > 99)
		mNumber = 0;

	mLCD->println(3," [%2d]", mNumber);
}

void cGetNumberMenu::handleDown()
{
	if(++mNumber > 99)
		mNumber = 0;

	mLCD->println(3," [%2d]", mNumber);
}

cGetNumberMenu::~cGetNumberMenu()
{
}

