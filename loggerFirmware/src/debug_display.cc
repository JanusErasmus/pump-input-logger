#include <cyg/kernel/diag.h>
#include <stdio.h>

#include "debug_display.h"

cDebugDisplay::cDebugDisplay()
{
	mCurrentLine = 1;
}

void cDebugDisplay::clear()
{
	diag_printf("-------debug display----------\n");
	mCurrentLine = 1;
}

void cDebugDisplay::println(cyg_uint8 line, const char *f,...)
{
	while(mCurrentLine > line)
	{
		diag_printf("\n");
		mCurrentLine++;
	}

	char tempString[128];

	va_list vl;
	va_start(vl,f);
	vsprintf(tempString, f,vl);
	va_end(vl);

	diag_printf("%s\n", tempString);
		mCurrentLine++;
}

cDebugDisplay::~cDebugDisplay()
{

}
