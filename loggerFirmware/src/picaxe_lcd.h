#ifndef PICAXE_LCD_H_
#define PICAXE_LCD_H_
#include <cyg/kernel/kapi.h>
#include <cyg/io/io.h>

#include "definitions.h"
#include "term.h"
#include "debug.h"

#define CFG_BUFF_SIZE 128

class cPICAXEserialLCD : public cDebug
{
	static cPICAXEserialLCD* _instance;

    cyg_io_handle_t mSerCMDHandle;


    cPICAXEserialLCD(char* serDev);

    void print(cyg_uint8* buff, cyg_uint8 len);
    void printCmd(cyg_uint8 cmd);
    void print(const char *string);

    void banner();

public:
	static void init(char* serDev);
	static cPICAXEserialLCD* get();

	void clear();
	void hide();
	void restore();
	void setCursor(cyg_uint8 row, cyg_uint8 col);
	void showCursor(cyg_uint8 row = 0xFF, cyg_uint8 col = 0xFF);
	void hideCursor();

	void println(cyg_uint8 line, const char* f,...);

	static void debugCMD(cTerm & term, int argc,char * argv[]);

	virtual ~cPICAXEserialLCD();
};





#endif /* PICAXE_LCD_H_ */
