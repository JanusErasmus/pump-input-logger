#ifndef SET_FRAME_MENU_H_
#define SET_FRAME_MENU_H_

#include "menu_lcd.h"

class cSetFrameMenu : public cLCDmenu
{
	cyg_uint8 mCursurPos;

public:
	cSetFrameMenu(cPICAXEserialLCD* lcd, cLCDmenu* parent = 0);

	void open();

	void handleEnter();
	void handleCancel();
	void handleUp();
	void handleDown();

	void returnParentMenu();

	virtual ~cSetFrameMenu();
};

#endif /* SET_TIME_MENU_H_ */
