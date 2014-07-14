#include <stdio.h>
#include <cyg/kernel/diag.h>
#include <cyg/kernel/kapi.h>
#include "debug.h"

//VT100 definitions
bool cDebug::VT100flag = false;
char cDebug::VT100_ID[] = { "\x1b[c" };
char cDebug::VT100_HOME[] = { "\x1b\x63" };
char cDebug::VT100_RED[] = { "\033[91m" };
char cDebug::VT100_BLUE[] = { "\x1b[94m" };
char cDebug::VT100_CYAN[] = { "\x1b[95m" };
char cDebug::VT100_YELLOW[]= { "\x1b[93m" };
char cDebug::VT100_GREEN[] = { "\x1b[32m" };
char cDebug::VT100_BLACKBACK[] = { "\x1b[40m" };
char cDebug::VT100_GREENBACK[] = { "\x1b[42m" };
char cDebug::VT100_NONE[] = { "\033[0m" };
char cDebug::VT100_UNDERLN[] = {  "\x1b[4m" };

cDebug::cDebug()
{
   mDebugLevel = 0;
}

void cDebug::setDebug(int lvl)
{
   mDebugLevel = lvl;
}

void cDebug::dbg_printf(int lvl,const char * fmt,...)
{
   if (mDebugLevel >= lvl)
   {
      va_list vl;
      va_start(vl,fmt);
      vsnprintf(str,256,fmt,vl);
      va_end(vl);
      diag_printf(str);
   }
}

void cDebug::dbg_dump_buf(int lvl, void * b,int len)
{
   if (mDebugLevel >= lvl)
   {
      diag_dump_buf(b,len);
   }

}

void cDebug::dbg_printf(sVT100command command,const char * fmt,...)
{
	char* startCMD = VT100_NONE;
	char* endCMD = VT100_NONE;
	va_list vl;
	va_start(vl,fmt);
	vsnprintf(str,256,fmt,vl);
	va_end(vl);
	if(VT100flag)
	{
		switch(command)
		{
		case clearScreen:
			startCMD = VT100_HOME;
			endCMD = 0;
			break;

		case red:
			startCMD = VT100_RED;
			endCMD = VT100_NONE;
			break;

		case green:
			startCMD = VT100_GREEN;
			endCMD = VT100_NONE;
			break;

		case blue:
			startCMD = VT100_BLUE;
			endCMD = VT100_NONE;
			break;

		case yellow:
			startCMD = VT100_YELLOW;
			endCMD = VT100_NONE;
			break;

		case cyan:
			startCMD = VT100_CYAN;
			endCMD = VT100_NONE;
			break;

		default:
			break;
		}

		diag_printf(startCMD);
		cyg_thread_delay(4);
		diag_printf(str);
		diag_printf(endCMD);

	}
	else
	{
		diag_printf(str);
	}

}

cDebug::~cDebug()
{

}



