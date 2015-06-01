#ifndef LOG_ACK_MENU_H_
#define LOG_ACK_MENU_H_
#include "menu_lcd.h"

class cLogAckMenu : public cLCDmenu
{
	cyg_uint8 mCursurPos;

public:
	cLogAckMenu(cLineDisplay * lcd, cLCDmenu* parent = 0);

	void open();

	void handleEnter();
	void handleCancel();
	void handleUp();
	void handleDown();

	virtual ~cLogAckMenu();
};

#endif /* LOG_ACK_MENU_H_ */
