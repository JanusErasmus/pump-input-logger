
#ifndef SRC_LINE_DISPLAY_H_
#define SRC_LINE_DISPLAY_H_
#include <cyg/kernel/kapi.h>

class cLineDisplay
{
public:
	cLineDisplay(){};
	virtual ~cLineDisplay(){};
	virtual void clear() = 0;
	virtual void setCursor(cyg_uint8 row, cyg_uint8 col) = 0;
	virtual void showCursor(cyg_uint8 row = 0xFF, cyg_uint8 col = 0xFF) = 0;
	virtual void hideCursor() = 0;
	virtual void println(cyg_uint8 line, const char *f,...) = 0;
};


#endif /* SRC_LINE_DISPLAY_H_ */
