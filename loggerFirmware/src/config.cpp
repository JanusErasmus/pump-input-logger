#include <cyg/kernel/diag.h>
#include <cyg/io/ttyio.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "config.h"
#include "nvm.h"
#include "utils.h"
#include "MCP_rtc.h"
#include "sys_mon.h"
#include "hobbs_timer.h"
#include "led.h"
#include "pwr_mon.h"
#include "temp_mon.h"
#include "log.h"

cConfig* cConfig::_instance = 0;

void cConfig::init(char* serDev)
{
	if(!_instance)
	{
		_instance = new cConfig(serDev);
	}
}

cConfig* cConfig::get()
{
	return _instance;
}

cConfig::cConfig(char* serDev)
{
	mRXbuff[0] = 0;
	mRXlen = 0;

	// Modem CMD UART
	Cyg_ErrNo err =	cyg_io_lookup(serDev,&mSerCMDHandle);

	diag_printf("cConfig %p: %s \n", mSerCMDHandle, strerror(-err));

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

	//diag_printf("Modem: TTY in_flags = 0x%08X, out_flags = 0x%08X\n",tty_info.tty_in_flags,tty_info.tty_out_flags);

	tty_info.tty_in_flags = 0;

	cyg_io_set_config(mSerCMDHandle,
			CYG_IO_SET_CONFIG_TTY_INFO,
			&tty_info,
			&len);

	cyg_mutex_init(&mRXmutex);

	cyg_thread_create(CFG_PRIOR,
			cConfig::rx_thread_func,
			(cyg_addrword_t)this,
			(char *)"Config",
			mStack,
			CFG_STACK_SIZE,
			&mThreadHandle,
			&mThread);

	cyg_thread_resume(mThreadHandle);

}

void cConfig::rx_thread_func(cyg_addrword_t arg)
{
	cConfig *t = (cConfig *)arg;

	for(;;)
	{
		t->run();
	}
}

void cConfig::run()
{
	mRXlen = CFG_BUFF_SIZE;
	Cyg_ErrNo err = cyg_io_read(mSerCMDHandle,mRXbuff,&mRXlen);
	if(err < 0)
		diag_printf("cModemRXerr: %s", strerror(-err));

	if(mRXlen > 0)
	{
		mRXbuff[mRXlen - 1] = 0;

		dbg_printf(1, "RX %d\n", mRXlen);
		dbg_dump_buf(2, mRXbuff, mRXlen);


		char *argv[20];
		int argc = 20;

		util_parse_params((char*)mRXbuff,argv,argc,'=',',');

		if ( argc )
		{
			//diag_printf("RX %d bytes\n", mRXlen);
			//diag_dump_buf(mRXbuff, mRXlen);
			handleAT(argv, argc);
		}

	}
}

void cConfig::handleAT(char *argv[],int argc)
{
	bool stat = false;

	if(!strcmp(argv[0], "AT"))
	{
		stat = true;
	}
	if(!strcmp(argv[0], "ATI"))
	{
		printCfg("\n");
		printCfg("Version: 0x%06X\n", cNVM::get()->getVersion());
		printCfg("Build: %s\n", cNVM::get()->getBuildDate());

		stat = true;
	}
	if(!strcmp(argv[0], "AT+TRANSFER"))	//set unit to transfer events
	{
		printCfg("\n");
		cSysMon::get()->setUploadDone(0);

		stat = true;
	}
	if(!strcmp(argv[0], "AT+STATUS"))	//get unit status
	{
		printCfg("\n");
		printCfg("RMMU status: %d\n", cLED::get()->getStatus());

		stat = true;
	}

	if(!strcmp(argv[0], "AT+DISCREET"))	//get discreet samples
	{
		printCfg("\n");
		printCfg("Input 0: %d\n", cPwrMon::get()->getPinStat(0));
		printCfg("Input 1: %d\n", cPwrMon::get()->getPinStat(1));

		stat = true;
	}
	if(!strcmp(argv[0], "AT+ANALOG"))	//get analog samples
	{
		float buff[4];
		if(cTempMon::get()->getSample(buff))
		{

			printCfg("\n");

			for(int k = 0; k < AN_IN_CNT; k++)
			{
				printCfg("INPUT %d: %.4f\n", k, buff[k]);
			}

			stat = true;
		}
		else
		{
			printCfg("\nTime not set\n");
		}
	}
	else if(!strcmp(argv[0], "AT+SYSMON"))	//print sysmon status
	{
		printCfg("\n");
		time_t t = cRTC::get()->timeNow();
		printCfg("TIME: %s", ctime(&t));
		t = cSysMon::get()->getLastSync();
		printCfg("SYNC: %s", ctime(&t));
		printCfg("PERIOD: %ds\n", cNVM::get()->getLogPeriod());

		stat = true;
	}
	else if(!strcmp(argv[0], "AT^CFG?"))	//print current config settings
	{
		printCfg("\n");
		printCfg("SN: 0x%08X\n", cNVM::get()->getSerial());
		printCfg("BOX: %010llu\n", cNVM::get()->getBoxSerial());
		printCfg("DIESEL: %d\n", cHobbsTimer::get()->getTime(0));
		printCfg("ELECTRIC: %d\n", cHobbsTimer::get()->getTime(1));
		printCfg("CELL: %s\n", cNVM::get()->getSimCell());
		printCfg("PIN: %s\n", cNVM::get()->getSimPin());
		printCfg("PUK: %s\n", cNVM::get()->getSimPuk());
		printCfg("APN: %s\n", cNVM::get()->getAPN());
		printCfg("USER: %s\n", cNVM::get()->getUser());
		printCfg("PASS: %s\n", cNVM::get()->getPasswd());
		printCfg("SERVER: %s\n", cNVM::get()->getServer());
		printCfg("PORT: %d\n", cNVM::get()->getPort());

		stat = true;
	}
	else if(!strcmp(argv[0], "AT^CFGSN"))	//set box number
		{
			if(argc > 1)
			{
				cyg_uint32 sn = strtoul(argv[1],NULL,16);
				cNVM::get()->setSerial(sn);

				printCfg("\n");
				printCfg("SN: 0x%08X\n", cNVM::get()->getSerial());

				stat = true;
			}
		}
	else if(!strcmp(argv[0], "AT^CFGBOX"))	//set box number
	{
		if(argc > 1)
		{
			cyg_uint64 box = atoll(argv[1]);
			cNVM::get()->setBoxSerial(box);

			printCfg("\n");
			printCfg("BOX: %010llu\n", cNVM::get()->getBoxSerial());

			stat = true;
		}
	}
	else if(!strcmp(argv[0], "AT^CFGDIESEL"))	//set dieselHobbs time
	{
		if(argc > 1)
		{
			cyg_uint32 diesel = atoi(argv[1]);
			diesel *= 3600;
			cHobbsTimer::get()->setTime(0, diesel);

			printCfg("\n");
			printCfg("DIESEL: %d\n", diesel);

			stat = true;
		}
	}
	else if(!strcmp(argv[0], "AT^CFGELECTRIC"))	//set electricHobbs time
	{
		if(argc > 1)
		{
			cyg_uint32 electric = atoi(argv[1]);
			electric *= 3600;
			cHobbsTimer::get()->setTime(1, electric);

			printCfg("\n");
			printCfg("ELECTRIC: %d\n", electric);

			stat = true;
		}
	}
	else if(!strcmp(argv[0], "AT^CFGSV"))	//set server
	{
		if(argc > 1)
		{
			cNVM::get()->setServer(argv[1]);

			printCfg("\n");
			printCfg("SERVER: %s\n", cNVM::get()->getServer());

			if(argc > 2)
			{
				cyg_uint32 port = strtoul(argv[2], NULL, 10);
				cNVM::get()->setPort(port);

				printCfg("PORT: %d\n", cNVM::get()->getPort());
			}

			stat = true;
		}
	}
	else if(!strcmp(argv[0], "AT^CFGPORT"))	//set port
	{
		if(argc > 1)
		{

			cyg_uint32 port = strtoul(argv[1], NULL, 10);
			cNVM::get()->setPort(port);

			printCfg("PORT: %d\n", cNVM::get()->getPort());


			stat = true;
		}
	}
	else if(!strcmp(argv[0], "AT^CFGAPN"))	//set APN
	{
		if(argc > 3)
		{
			cNVM::get()->setAPN(argv[1]);
			cNVM::get()->setUser(argv[2]);
			cNVM::get()->setPasswd(argv[3]);

			printCfg("\n");
			printCfg("APN: %s\n", cNVM::get()->getAPN());
			printCfg("USER: %s\n", cNVM::get()->getUser());
			printCfg("PASS: %s\n", cNVM::get()->getPasswd());

			stat = true;
		}
	}
	else if(!strcmp(argv[0], "AT^CFGCELL"))	//set cell number
	{
		if(argc > 1)
		{
			cNVM::get()->setSimCell(argv[1]);

			printCfg("\n");
			printCfg("CELL: %s\n", cNVM::get()->getSimCell());
			stat = true;
		}
	}
	else if(!strcmp(argv[0], "AT^CFGSIM"))	//set SIM numbers
	{
		if(argc > 1)
		{
			cNVM::get()->setSimPin(argv[1]);

			printCfg("\n");
			printCfg("PIN: %s\n", cNVM::get()->getSimPin());

			if(argc > 2)
			{
				cNVM::get()->setSimPuk(argv[2]);

				printCfg("PUK: %s\n", cNVM::get()->getSimPuk());
			}

			stat = true;
		}
	}
	else if(!strcmp(argv[0], "ATE1")) //enable echo
	{
		cyg_tty_info_t tty_info;
		cyg_uint32 len = sizeof(tty_info);
		cyg_io_get_config(mSerCMDHandle,
				CYG_IO_GET_CONFIG_TTY_INFO,
				&tty_info,
				&len);

		tty_info.tty_in_flags |= CYG_TTY_IN_FLAGS_ECHO;

		cyg_io_set_config(mSerCMDHandle,
				CYG_IO_SET_CONFIG_TTY_INFO,
				&tty_info,
				&len);

		stat = true;
	}
	else if(!strcmp(argv[0], "ATE0")) //disable echo
	{
		cyg_tty_info_t tty_info;
		cyg_uint32 len = sizeof(tty_info);
		cyg_io_get_config(mSerCMDHandle,
				CYG_IO_GET_CONFIG_TTY_INFO,
				&tty_info,
				&len);

		tty_info.tty_in_flags &= ~(CYG_TTY_IN_FLAGS_ECHO);

		cyg_io_set_config(mSerCMDHandle,
				CYG_IO_SET_CONFIG_TTY_INFO,
				&tty_info,
				&len);

		stat = true;
	}
	else if(!strcmp(argv[0], "AT^ACK"))
	{

		for(int k = 0; k < 100; k++)
		{
			cEvent e;
			if(cLog::get()->readEvent(&e))
			{
				sEventData data = e.getData();

				if(data.mType == cEvent::EVENT_TEMP)
				{
					printCfg("A, ");
					printCfg("0x%04X, ", data.mSeq);
					printCfg("%d, ", data.mPort);
					printCfg("%.1f,", data.mTemp);
					printCfg("%d,", data.mTime);
					printCfg(ctime((time_t*)&data.mTime));
				}

				if(data.mType == cEvent::EVENT_INPUT)
				{
					printCfg("D, ");
					printCfg("0x%04X, ", data.mSeq);
					printCfg("%d, ", data.mPort);
					printCfg("%d,", data.mState);
					printCfg("%d,", data.mTime);
					printCfg(ctime((time_t*)&data.mTime));
				}

				cLog::get()->acknowledge();
				cLog::get()->readNext();
			}
		}
		stat = true;
	}

	if(stat)
	{
		printCfg("\nOK\n");
	}
	else
	{
		printCfg("\nERROR\n");
	}
}

cyg_uint8 cConfig::printCfg(const char *f,...)
{
    va_list vl;
    va_start(vl,f);
    vsprintf(mTXbuff,f,vl);
    va_end(vl);


	cyg_uint32 len = strlen(mTXbuff);
	dbg_printf(1, "Sending %d\n", len);
	dbg_dump_buf(2, mTXbuff, len);
	Cyg_ErrNo err = cyg_io_write(mSerCMDHandle,mTXbuff,&len);
	if(err < 0)
		diag_printf("cModemTXerr: %s", strerror(-err));

	return (cyg_uint8)len;
}

cConfig::~cConfig()
{
}

