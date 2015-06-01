#ifndef SRC_DEBUG_DISPLAY_H_
#define SRC_DEBUG_DISPLAY_H_

#include "line_display.h"

class cDebugDisplay : public cLineDisplay
{
	cyg_uint8 mCurrentLine;

public:
	cDebugDisplay();
	virtual ~cDebugDisplay();

	void clear();
	void setCursor(cyg_uint8 row, cyg_uint8 col){};
	void showCursor(cyg_uint8 row = 0xFF, cyg_uint8 col = 0xFF){};
	void hideCursor(){};
	void println(cyg_uint8 line, const char *f,...);
};

#endif /* SRC_DEBUG_DISPLAY_H_ */
