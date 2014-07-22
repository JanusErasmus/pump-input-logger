#ifndef LOG_ACK_MENU_H_
#define LOG_ACK_MENU_H_
#include "lcd_menu.h"

class cLogAckMenu : public cLCDmenu
{

public:
	cLogAckMenu(cPICAXEserialLCD* lcd, cLCDmenu* parent = 0);

	void open();

	void handleEnter();
	void handleCancel();

	virtual ~cLogAckMenu();
};

#endif /* LOG_ACK_MENU_H_ */
