#include <cyg/kernel/diag.h>
#include <cyg/io/ttyio.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "picaxe_lcd.h"
#include "version.h"

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
	info.baud = CYGNUM_SERIAL_BAUD_2400;
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

	//banner();
}

void cPICAXEserialLCD::banner()
{
	clear();
	println(1, "cPICAXEserialLCD");
	println(2, "Ver: %d.%d.%d",(VERSION_NUM & 0xFF0000)>>16,(VERSION_NUM & 0xFF00)>>8,(VERSION_NUM & 0xFF));
}

void cPICAXEserialLCD::setCursor(cyg_uint8 row, cyg_uint8 col)
{
	if(row == 0xFF || col == 0xFF)
		return;

	if(col > 19)
		col = 19;

	switch(row)
	{
	default:
	case 1:
		printCmd(0x80);
		break;
	case 2:
		printCmd(0xC0);
		break;
	case 3:
		printCmd(0x94);
		break;
	case 4:
		printCmd(0xD4);
		break;

	}
}

void cPICAXEserialLCD::showCursor(cyg_uint8 row, cyg_uint8 col)
{
	setCursor(row, col);
	printCmd(0x0D);
}

void cPICAXEserialLCD::hideCursor()
{
	printCmd(0x0C);
}

void cPICAXEserialLCD::clear()
{
	printCmd(0x01);
}

void cPICAXEserialLCD::hide()
{
	printCmd(0x08);
}

void cPICAXEserialLCD::restore()
{
	printCmd(0x0C);
}

void cPICAXEserialLCD::print(cyg_uint8* buff, cyg_uint8 len)
{
	for(cyg_uint8 k = 0; k < len; k++)
	{
		cyg_uint32 tx = 1;
		Cyg_ErrNo err = cyg_io_write(mSerCMDHandle, &buff[k], &tx);
		if(err < 0)
		{
			diag_printf("PICAXE TXerr: %s", strerror(-err));
			return;
		}
		cyg_thread_delay(6);
	}
}

void cPICAXEserialLCD::printCmd(cyg_uint8 cmd)
{
	cyg_uint8 buff[2];
	buff[0] = 254;
	buff[1] = cmd;

	print(buff, 2);
}

void cPICAXEserialLCD::print(const char *string)
{
	cyg_uint8 len = strlen(string);
	if(len > 20)
		len = 20;

	print((cyg_uint8*)string, len);
}

void cPICAXEserialLCD::println(cyg_uint8 line, const char *f, ...)
{
	char buff[64];
	va_list vl;
	va_start(vl,f);
	vsprintf(buff,f,vl);
	va_end(vl);

	setCursor(line, 0);
	print(buff);
}

void cPICAXEserialLCD::debugCMD(cTerm & term, int argc,char * argv[])
{
	_instance->clear();
	_instance->hide();
	_instance->println(1, "LINE 1...baie lang string word geprint->");
	_instance->println(2, "LINE 2  ->");
	_instance->println(3, "LINE 3  ;-)");
	_instance->println(4, "LINE 4  :-0");
	_instance->restore();
}

cPICAXEserialLCD::~cPICAXEserialLCD()
{
}

