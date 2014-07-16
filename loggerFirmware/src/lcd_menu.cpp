#include "lcd_menu.h"

cLCDmenu::cLCDmenu(cPICAXEserialLCD* lcd, const char* heading) :  mOpened(false), mHeading(heading), mLCD(lcd)
{
}


cLCDmenu::~cLCDmenu()
{
}

