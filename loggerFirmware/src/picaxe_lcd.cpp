#include <cyg/kernel/diag.h>
#include <cyg/io/ttyio.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "picaxe_lcd.h"

cPICAXEserialLCD* cPICAXEserialLCD::_instance = 0;

void cPICAXEserialLCD::init(char* serDev)
{
	if(!_instance)
	{
		_instance = new cPICAXEserialLCD(serDev);
	}
}

cPICAXEserialLCD* cPICAXEserialLCD::get()
{
	return _instance;
}

cPICAXEserialLCD::cPICAXEserialLCD(char* serDev)
{
	mDebugLevel = 4;

	// Modem CMD UART
	Cyg_ErrNo err =	cyg_io_lookup(serDev,&mSerCMDHandle);

	diag_printf("cPICAXEserialLCD %p: %s \n", mSerCMDHandle, strerror(-err));

	cyg_serial_info_t info;

	info.flags = 0;
	info.baud = CYGNUM_SERIAL_BAUD_115200;
	info.stop = CYGNUM_SERIAL_STOP_1;
	info.parity = CYGNUM_SERIAL_PARITY_NONE;
	info.word_length = CYGNUM_SERIAL_WORD_LENGTH_8;

	cyg_uint32 info_len = sizeof(info);
	cyg_io_set_config(mSerCMDHandle,
			CYG_IO_SET_CONFIG_SERIAL_INFO,
			&info,
			&info_len);

	cyg_tty_info_t tty_info;
	cyg_uint32 len = sizeof(tty_info);
	cyg_io_get_config(mSerCMDHandle,
			CYG_IO_GET_CONFIG_TTY_INFO,
			&tty_info,
			&len);

	//diag_printf("cPICAXEserialLCD: TTY in_flags = 0x%08X, out_flags = 0x%08X\n",tty_info.tty_in_flags,tty_info.tty_out_flags);

	tty_info.tty_in_flags = 0;

	cyg_io_set_config(mSerCMDHandle,
			CYG_IO_SET_CONFIG_TTY_INFO,
			&tty_info,
			&len);
}

void cPICAXEserialLCD::debugCMD(cTerm & term, int argc,char * argv[])
{
	char str[] = {"hello"};
	cyg_uint32 len = strlen(str);
	_instance->dbg_printf(3, "PICAXE: (%d) - %s \n", len, str);

	Cyg_ErrNo err = cyg_io_write(_instance->mSerCMDHandle, str, &len);
	if(err < 0)
		diag_printf("PICAXE TXerr: %s", strerror(-err));
}

cPICAXEserialLCD::~cPICAXEserialLCD()
{
}

