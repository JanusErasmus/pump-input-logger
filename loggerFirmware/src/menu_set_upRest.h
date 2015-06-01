#ifndef SET_UP_REST_MENU_H_
#define SET_UP_REST_MENU_H_

#include "menu_lcd.h"

class cSetUpRestMenu : public cLCDmenu
{
	cyg_uint8 mCursurPos;

public:
	cSetUpRestMenu(cLineDisplay * lcd, cLCDmenu* parent = 0);

	void open();

	void handleEnter();
	void handleCancel();
	void handleUp();
	void handleDown();

	void returnParentMenu();

	virtual ~cSetUpRestMenu();
};

#endif /* SET_UP_REST_MENU_H_ */
