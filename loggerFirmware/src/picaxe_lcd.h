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

public:
	static void init(char* serDev);
	static cPICAXEserialLCD* get();

	static void debugCMD(cTerm & term, int argc,char * argv[]);

	virtual ~cPICAXEserialLCD();
};





#endif /* PICAXE_LCD_H_ */
