#include "TermCMD.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "nvm.h"
#include "spi_flash.h"
#include "MCP_rtc.h"
#include "pwr_mon.h"
#include "temp_mon.h"
#include "modem.h"
#include "event.h"
#include "log.h"
#include "hobbs_timer.h"
#include "sys_mon.h"
#include "config.h"

TermCMD::cmd_table_t TermCMD::mCmdTable [] =
{
	{"SYSTEM"	,0,0,0},
    {"h"	, "",			"Show this help info", TermCMD::help},
    {"help"	, "",			"Show this help info", TermCMD::help},
    {"dt"	, "",			"Dump Thread Info", System::handle}, 		//Used from previous command
    {"debug", "<mod> <lvl>","Set the Debug level for a module, 0 disables debug output", System::setDebugLvl},
    {"ram"	, "",			"Show RAM usage", System::handle},		//Used from previous command
    {"time"	, "",			"Current system time", System::handle},	//Used from previous command
    {"cls"	, "",			"Clear the screen", System::handle},	//Used from previous command
    {"reset"	, "",			"Reset processor", System::reset},
    {"VERSION & SERIAL"	,0,0,0},
    {"hw"	, "<num>",		"Set the Hardware revision", System::handle},//Used from previous command
    {"ver"	, "",			"Show the build and commit dates for this binary", System::handle},//Used from previous command
    {"sn"	, "",			"Set 64bit serial# adress LSB first", System::handle},//Used from previous command
    {"box"	, "",			"Set 64bit box# adress LSB first", System::handle},//Used from previous command
    {"SPI FLASH"	,0,0,0},
    {"flrd"	, "<addr> <num>","Read num bytes from serial flash at addr", System::flashCmd},//Used from previous command
    {"flwr"	, "<addr> <num> <val>", "Write num bytes of val to serial flash at addr", System::flashCmd},//Used from previous command
    {"fle"	, "<addr>",		"Erase serial flash sector containing addr", System::flashCmd},//Used from previous command
    {"RTC TIME"	,0,0,0},
    {"syncTime", "",		"Sync system time with MCP RTC", MCP::sync},
    {"setRTC", "<yyyy mm dd HH MM SS>","Set MCP RTC", MCP::set},
    {"powerChange", "","Show when unit power was changed", MCP::powerChange},
    {"Inputs"	,0,0,0},
    {"disc", "",			"Show discrete inputs", inputs::discrete},
    {"ana", "",				"Show analog inputs", inputs::analog},
    {"m", "",				"Show analog measurements", inputs::showM},
    {"incHobbs", "<port>",	"increment Hobbs timer", inputs::incHobbs},
    {"showHobbs", "",		"Show Hobbs times", inputs::showHobbs},
    {"MODEM"	,0,0,0},
    {"AT****", "",			"Send modem cmd (CAPITALIZED)", modem::ATcmd},
    {"cmd", "",				"test modem cmd", modem::cmd},
    {"fixBaud", "",				"Fix Baud rate to 115200", modem::fixBaud},
    {"modemStat", "",		"Get modem status", modem::status},
    {"modemReset", "",		"Hard reset the modem", modem::reset},
    {"SYSMON"	,0,0,0},
    {"pwrModem", "<stat>",	"Set SYSMON pwrStat and Press modem PWR_KEY", systemMon::setPowerStat},
    {"setPeriod", "<period>","Set SYSMON update period", systemMon::setPeriod},
    {"sync", "",			"Synchronise logs to server", systemMon::sync},
    {"syncDone", "",		"Simulate a good transfer", systemMon::syncDone},
    {"sysmon", "",			"Show SYSMON status", systemMon::stat},
    {"LOGGER"	,0,0,0},
    {"log", "<log count>",	"log a event", logEvent::log},
    {"logD", "<log count>",	"log a event", logEvent::logDisc},
    {"ack", "<log count>",	"acknowledge a event", logEvent::ack},
    {"showlog", "",			"Show all loged a events", logEvent::showAll},
    {"NVM"	,0,0,0},
    {"nvm"	,"",			"Displays the port default values", System::nvmBuff},
    {"setUpper"	,"<port> <val>",	"Set port Upper limit", System::setUpperLimit},
    {"setLower"	,"<port> <val>",	"Set port Upper limit", System::setLowerLimit},
    {"cfg"	,"<param to change>","Configures the apn port values", System::config},
    {0, 0, 0, 0}
};

void logEvent::showAll(cTerm & term, int argc,char * argv[])
{
	cLog::get()->showLogs();
}

void logEvent::log(cTerm & term, int argc,char * argv[])
{
	cyg_uint8 cnt;
	if(argc > 1)
	{
		cnt = strtoul(argv[1],NULL,10);
		for(int k = 0; k < cnt; k++)
		{
			cEvent e(4, (float)25, cRTC::get()->timeNow());
			cLog::get()->logEvent(&e);
			e.showEvent();

			if(argc == 2)
				cyg_thread_delay(500);
		}
	}
	else
	{
		cEvent e(4, (float)25, cRTC::get()->timeNow());
		cLog::get()->logEvent(&e);
		e.showEvent();
	}
}

void logEvent::logDisc(cTerm & term, int argc,char * argv[])
{

	cyg_uint8 cnt;
	if(argc > 1)
		{
		cyg_uint8 state = 0;
			cnt = strtoul(argv[1],NULL,10);
			for(int k = 0; k < cnt; k++)
			{
				cEvent e(1, state, cRTC::get()->timeNow());
				cLog::get()->logEvent(&e);
				e.showEvent();

				state = (~state & 0x01);
				cyg_thread_delay(500);
			}
		}
	else
	{
		cEvent e(0, (cyg_uint8)1, cRTC::get()->timeNow());
		cLog::get()->logEvent(&e);
		e.showEvent();
	}
}

void logEvent::ack(cTerm & term, int argc,char * argv[])
{
	cyg_uint8 cnt;
	if(argc > 1)
		{
			cnt = strtoul(argv[1],NULL,10);
			for(int k = 0; k < cnt; k++)
			{
				cEvent e;
				if(cLog::get()->readEvent(&e))
				{
					cLog::get()->acknowledge();
					cLog::get()->readNext();
				}
			}
		}
	else
	{
		cEvent e;
		if(cLog::get()->readEvent(&e))
		{
			e.showEvent();
			cLog::get()->acknowledge();
			cLog::get()->readNext();
		}
	}
}

void modem::ATcmd(cTerm & term, int argc,char * argv[])
{
	char atCMD[32];
	sprintf(atCMD,"%s\n", argv[0]);

	cModem::get()->write(atCMD);
}

void modem::cmd(cTerm & term, int argc,char * argv[])
{
	cModem::get()->doCMD();
}

void modem::status(cTerm & term, int argc,char * argv[])
{
	cModem::eModemStat stat = cModem::get()->getStatus();
	cModem::get()->showStatus(stat);
}

void modem::reset(cTerm & term, int argc,char * argv[])
{
	cModem::get()->reset();
}

void modem::fixBaud(cTerm & term, int argc,char * argv[])
{
	cModem::get()->setFixedBaud();
}

void systemMon::sync(cTerm & term, int argc,char * argv[])
{
	cSysMon::get()->setUploadDone(0);
}

void systemMon::syncDone(cTerm & term, int argc,char * argv[])
{
	cSysMon::get()->setUploadDone();
}

void systemMon::stat(cTerm & term, int argc,char * argv[])
{
	cSysMon::get()->showStat();
}

void systemMon::setPeriod(cTerm & term, int argc,char * argv[])
{
	if(argc > 1)
	{
		cyg_uint32 per = strtoul(argv[1],NULL,10);
		term<<term.format("Set period to: %ds\n", per);
		cNVM::get()->setLogPeriod(per);
	}
}

void systemMon::setPowerStat(cTerm & term, int argc,char * argv[])
{
	if(argc > 1)
	{
		bool flag = strtoul(argv[1],NULL,10);
		term<<"pressing modem pwrKey button - ";

		if(flag)
		{
			term<<"on\n";
		}
		else
		{
			term<<"off\n";
		}

		cModem::get()->power(flag);
		cSysMon::get()->setPowerStat(flag);
	}
}

void inputs::discrete(cTerm & term, int argc,char * argv[])
{
	if(cPwrMon::get()->getPinStat(0))
		term.dbg_printf(green,"Input 0: ON\n");
	else
		term.dbg_printf(red,"Input 0: OFF\n");

	if(cPwrMon::get()->getPinStat(1))
		term.dbg_printf(green,"Input 1: ON\n");
	else
		term.dbg_printf(red,"Input 1: OFF\n");
}


void inputs::analog(cTerm & term, int argc,char * argv[])
{
	float buff[4];
	if(cTempMon::get()->getSample(buff))
	{

		for(int k = 0; k < AN_IN_CNT; k++)
		{
			printf("buff[%d]: %.4f\n", k, buff[k]);
		}
	}
	else
	{
		diag_printf("Time not set...\n");
	}
}

void inputs::showM(cTerm & term, int argc,char * argv[])
{
	cTempMon::get()->showMeasurements();
}

void inputs::incHobbs(cTerm & term, int argc,char * argv[])
{
	if(argc > 2)
	{
		int p = strtoul(argv[1],NULL,10);
		int cnt = strtoul(argv[2],NULL,10);
		for(int k = 0; k < cnt; k++)
		{
			cHobbsTimer::get()->incTime(p, 60);
		}
	}
	else if(argc > 1)
	{
		int p = strtoul(argv[1],NULL,10);
		cHobbsTimer::get()->incTime(p, 60);
	}
	else
	{
		term << "Set port to increment\n";
	}
}

void inputs::showHobbs(cTerm & term, int argc,char * argv[])
{
	cHobbsTimer::get()->showTimes();
}

void MCP::set(cTerm & term, int argc,char * argv[])
{
	cyg_uint16 yy,mm,dd,h,m,s;
	if(argc > 5)
	{
		yy = strtoul(argv[1],NULL,10);
		if(yy < 1900)
			return;
		diag_printf("yy : %d\n", yy);

		mm = strtoul(argv[2],NULL,10);
		if(mm > 12)
			return;
		diag_printf("mm : %d\n", mm);

		dd = strtoul(argv[3],NULL,10);
		if(dd > 31)
			return;
		diag_printf("dd : %d\n", dd);

		h = strtoul(argv[4],NULL,10);
		if(h > 24)
			return;
		diag_printf("h  : %d\n", h);

		m = strtoul(argv[5],NULL,10);
		if(m > 60)
			return;
		diag_printf("m  : %d\n", m);

		s = strtoul(argv[6],NULL,10);
		if(s > 60)
			return;
		diag_printf("s  : %d\n", s);

		cRTC::get()->setTime(yy, mm, dd, h, m, s);

		term<<"Updated time\n";
	}
	else
	{
		term<<"Not enough input values. See help\n";
	}


}

void MCP::sync(cTerm & term, int argc,char * argv[])
{
	cRTC::get()->syncTime();
	term.dbg_printf(green,"Time synchronized:\n");
}


void MCP::powerChange(cTerm & term, int argc, char *argv[])
{
	term.dbg_printf(green,"Power change status:\n");
	time_t t = cRTC::get()->getPowerDown();
	term << "Power Down: " << ctime(&t);
	t = cRTC::get()->getPowerUp();
	term << "Power Up  : " << ctime(&t);
}

void System::nvmBuff(cTerm & t,int argc,char *argv[])
{
	t.dbg_printf(green,"Default status:\n");
	for(int k = 0; k < 4; k++)
	{
		printf("%d: %0.1f %0.1f %0.1f\n", k, cNVM::get()->getSampleRange(k) , cNVM::get()->getUpperLimit(k), cNVM::get()->getLowerLimit(k));
	}
}

void System::setUpperLimit(cTerm & t,int argc,char *argv[])
{
	t.dbg_printf(green,"Set UpperLimit:\n");
	if (argc > 2)
	{
		int port = atoi(argv[1]);
		int val = atoi(argv[2]);
		t<<"Port "<<port<<" val: "<<val<<"\n";

		cNVM::get()->setUpperLimit(port, val);
	}

}

void System::setLowerLimit(cTerm & t,int argc,char *argv[])
{
	t.dbg_printf(green,"Set LowerLimit:\n");
	if (argc > 2)
	{
		int port = atoi(argv[1]);
		int val = atoi(argv[2]);
		t<<"Port "<<port<<" val: "<<val<<"\n";

		cNVM::get()->setLowerLimit(port, val);
	}

}

void System::config(cTerm & t,int mArgc,char *mArgv[])
{
	t.dbg_printf(yellow, "Server configuration:\n");

	int arg_idx = 1;
	while(arg_idx < mArgc)
	{
		if(!strcmp("apn",mArgv[arg_idx]))
		{
			// Move onto the parameter//
			arg_idx++;
			if(arg_idx < mArgc)
			{
				// We have a parameter //
				t<<"Setting APN to "<<mArgv[arg_idx]<<"\n";
				cNVM::get()->setAPN(mArgv[arg_idx]);
			}
			else
			{
				// Just print the APN name //
				t<<"APN="<<cNVM::get()->getAPN()<<"\n";
				return;
			}
		}
		else if(!strcmp("user",mArgv[arg_idx]))
		{
			// Move onto the parameter//
			arg_idx++;
			if(arg_idx < mArgc)
			{
				// We have a parameter //
				t<<"Setting User to "<<mArgv[arg_idx]<<"\n";
				cNVM::get()->setUser(mArgv[arg_idx]);
			}
			else
			{
				// Just print the User name //
				t<<"USER="<<cNVM::get()->getUser()<<"\n";
				return;
			}
		}
		else if(!strcmp("passwd",mArgv[arg_idx]))
		{
			// Move onto the parameter//
			arg_idx++;
			if(arg_idx < mArgc)
			{
				// We have a parameter //
				t<<"Setting Password to "<<mArgv[arg_idx]<<"\n";
				cNVM::get()->setPasswd(mArgv[arg_idx]);
			}
			else
			{
				// Just print the password name //
				t<<"PASSWD="<<cNVM::get()->getPasswd()<<"\n";
				return;
			}
		}
		else if(!strcmp("server",mArgv[arg_idx]))
		{
			// Move onto the parameter//
			arg_idx++;
			if(arg_idx < mArgc)
			{
				// We have a parameter //
				t<<"Setting server to "<<mArgv[arg_idx]<<"\n";
				cNVM::get()->setServer(mArgv[arg_idx]);
			}
			else
			{
				// Just print the Server name //
				t<<"SERVER="<<cNVM::get()->getServer()<<"\n";
				return;
			}
		}
		else if(!strcmp("cell",mArgv[arg_idx]))
				{
					// Move onto the parameter//
					arg_idx++;
					if(arg_idx < mArgc)
					{
						// We have a parameter //
						t<<"Setting SIM cell to "<<mArgv[arg_idx]<<"\n";
						cNVM::get()->setSimCell(mArgv[arg_idx]);
					}
					else
					{
						// Just print the Server name //
						t<<"SIM CELL="<<cNVM::get()->getSimCell()<<"\n";
						return;
					}
				}
		else if(!strcmp("pin",mArgv[arg_idx]))
		{
			// Move onto the parameter//
			arg_idx++;
			if(arg_idx < mArgc)
			{
				// We have a parameter //
				t<<"Setting SIM pin to "<<mArgv[arg_idx]<<"\n";
				cNVM::get()->setSimPin(mArgv[arg_idx]);
			}
			else
			{
				// Just print the Server name //
				t<<"SIM PIN="<<cNVM::get()->getSimPin()<<"\n";
				return;
			}
		}
		else if(!strcmp("puk",mArgv[arg_idx]))
		{
			// Move onto the parameter//
			arg_idx++;
			if(arg_idx < mArgc)
			{
				// We have a parameter //
				t<<"Setting SIM puk to "<<mArgv[arg_idx]<<"\n";
				cNVM::get()->setSimPuk(mArgv[arg_idx]);
			}
			else
			{
				// Just print the Server name //
				t<<"SIM PUK="<<cNVM::get()->getSimPuk()<<"\n";
				return;
			}
		}
		else if(!strcmp("port",mArgv[arg_idx]))
		{
			// Move onto the parameter//
			arg_idx++;
			int port;
			if(arg_idx < mArgc)
			{
				// We have a parameter //
				port = atoi(mArgv[arg_idx]);
				t<<"Setting port to "<<port<<"\n";
				cNVM::get()->setPort(port);
			}
			else
			{
				// Just print the Server name //
				t<<"PORT="<<(int)cNVM::get()->getPort()<<"\n";
				return;
			}
		}
		else if(!strcmp("default",mArgv[arg_idx]))
		{
			// Move onto the parameter//
			t<<"Setting configuration defaults ...\n";
			cNVM::get()->setDefault();
		}

		else
		{
			t<<"Unknown parameter "<<mArgv[arg_idx]<<"\n";
		}
		// move onto next param descriptor //
		arg_idx++;
	}
	t<<t.format("%12s","SIM CELL = ")	<<cNVM::get()->getSimCell()<<"\n";
	t<<t.format("%12s","SIM PIN = ")	<<cNVM::get()->getSimPin()<<"\n";
	t<<t.format("%12s","SIM PUK = ")	<<cNVM::get()->getSimPuk()<<"\n";
	t<<"\n";
	t<<t.format("%12s","APN = ")		<<cNVM::get()->getAPN()<<"\n";
	t<<t.format("%12s","USER = ")		<<cNVM::get()->getUser()<<"\n";
	t<<t.format("%12s","PASSWD = ")		<<cNVM::get()->getPasswd()<<"\n";
	t<<t.format("%12s","SERVER = ")		<<cNVM::get()->getServer()<<"\n";
	t<<t.format("%12s","PORT = ")		<<t.format("%04d", cNVM::get()->getPort())<<"\n";

}

void System::handle(cTerm & t,int argc,char *argv[])
{
	if (!strcmp(argv[0],"dt"))
	{
		cyg_handle_t thread = 0;
		cyg_uint16 id;
		cyg_thread_info info;
		bool flag = 0;

		if(cDebug::VT100flag)
		{
			t<<cDebug::VT100_HOME;
			cyg_thread_delay(2);
			t<<cDebug::VT100_UNDERLN;
			t<<cDebug::VT100_BLUE;
		}
		t<<" ID"<<t.format("% 15s","Name")<<t.format("% 6s","Prior")<<t.format("% 12s","S_Size")<<t.format("% 12s","Used")<<t.format("% 5s","Perc")<<"\n";

		if(cDebug::VT100flag)
			t<<cDebug::VT100_NONE;

		while ( cyg_thread_get_next(&thread,&id) )
		{
			if ( !cyg_thread_get_info(thread,id,&info) )
			{
				break;
			}
			if(flag && cDebug::VT100flag)
				t<<cDebug::VT100_RED;
			t<<t.format("% 2d",info.id)<<t.format("% 15s",info.name)<<t.format("% 6d",(int)info.set_pri)<<t.format("  0x%08X",info.stack_size)<<t.format("  0x%08X",info.stack_used)<<t.format("% 5d",(int)((info.stack_used*100)/info.stack_size))<<"\n";
			flag = !flag;

			if(cDebug::VT100flag)
				t<<cDebug::VT100_NONE;
		}

		return;
	}

	if (!strcmp(argv[0],"ram"))
	{
		extern cyg_uint32  __rom_data_start;	//diag_printf("ROMstart 0x%08X\n",(cyg_uint32)&__rom_data_start);
		//extern cyg_uint32  __rom_data_end;		//diag_printf("ROMend   0x%08X\n",(cyg_uint32)&__rom_data_end);


		extern cyg_uint32  __ram_data_start;	//diag_printf("RAMstart 0x%08X\n",(cyg_uint32)&__ram_data_start);
		extern cyg_uint32  __ram_data_end;		//diag_printf("RAMend   0x%08X\n",(cyg_uint32)&__ram_data_end);
		extern cyg_uint32  _end;				//diag_printf("__end    0x%08X\n",(cyg_uint32)&_end);
		struct mallinfo heap_info = mallinfo();

		cyg_uint32 text_size = (cyg_uint32)&__rom_data_start - 0x08000000;
		cyg_uint32 data_size =  (cyg_uint32)&__ram_data_end - (cyg_uint32)&__ram_data_start;
		cyg_uint32 bss_size  = (cyg_uint32)&_end - (cyg_uint32)&__ram_data_end;
		cyg_uint32 total_ram = text_size + data_size + bss_size;

		if(cDebug::VT100flag)
			t<<cDebug::VT100_GREENBACK;
		t<<"Program memory:";
		if(cDebug::VT100flag)
			t<<cDebug::VT100_NONE;
		t<<"\n";

		t<<".text = "<<text_size<<"\n";
		t<<".data = "<<data_size<<"\n";
		t<<" .bss = "<<bss_size<<"\n";
		t<<"total = "<<total_ram<<"\n";
		t<<" perc = "<<(int)((total_ram*100)/(0x80000-0x180))<<"%\n"<<"\n";

		if(cDebug::VT100flag)
			t<<cDebug::VT100_GREENBACK;
		t<<"RAM:";
		if(cDebug::VT100flag)
			t<<cDebug::VT100_NONE;
		t<<"\n";

		t<<".heap = "<<heap_info.arena<<"\n";
		t<<" Used = "<<heap_info.usmblks+heap_info.uordblks<<"\n";
		t<<" perc = "<<(int)(((heap_info.usmblks+heap_info.uordblks)*100)/heap_info.arena)<<"%\n"<<"\n";

		return;
	}


	if(!strcmp(argv[0],"ver"))
	{
		//t<<"Hardware Version  : "<<cNVM::get()->getHWRev()<<"\n";
		printf("Version: 0x%06X\n", cNVM::get()->getVersion());
		t<<"Build Timestamp   : "<<cNVM::get()->getBuildDate()<<"\n";
	}

	if(!strcmp(argv[0],"time"))
	{
		t.dbg_printf(green,"Current time\n");

		time_t now = time(NULL);
		t<<"eCos: "<<asctime(localtime(&now));
		now = cRTC::get()->timeNow();
		t<<"RTC : "<<asctime(localtime(&now));
	}


	if (!strcmp("sn", argv[0]))
	{
		cyg_uint32 sn;
		if(argc > 1)
		{
			sn = strtoul(argv[1],NULL,16);
			cNVM::get()->setSerial(sn);
			t<<"Changed Serial number to "<<sn<<"\n";
		}
		else
		{
			sn = cNVM::get()->getSerial();
			t<<"Unit serial number is "<<sn<<"\n";
		}
		return;
	}

	if (!strcmp("box", argv[0]))
		{
			cyg_uint64 box;
			if(argc > 1)
			{
				box = atoll(argv[1]);
				cNVM::get()->setBoxSerial(box);
				t<<t.format("Changed Box number to %010llu\n",box);
			}
			else
			{
				box = cNVM::get()->getBoxSerial();
				t<<t.format("Unit Box number is %010llu\n", box);
			}
			return;
		}

	if(!strcmp(argv[0],"hw"))
	{
		if(argc > 1)
		{
			cNVM::get()->setHWRev(argv[1]);
		}
		else
		{
			t<<"Hardware revision: "<<cNVM::get()->getHWRev();

		}
	}

	if (!strcmp(argv[0],"cls"))
	{
		if(cDebug::VT100flag)
		{
			t<<cDebug::VT100_HOME;
			cyg_thread_delay(1);
		}
	}

}

void System::setDebugLvl(cTerm & t,int argc,char *argv[])
{

	if (!strcmp("debug",argv[0]))
	{
		if (argc > 2)
		{
			int lvl = atoi(argv[2]);
			if(!strcmp("flash",argv[1]))
			{
				t<<"Setting SpiFlash debug to "<<lvl<<"\n";
				SpiFlash::get()->setDebug(lvl);
			}
			else if(!strcmp("rtc",argv[1]))
			{
				t<<"Setting RTC debug to "<<lvl<<"\n";
				cRTC::get()->setDebug(lvl);
			}
			else if(!strcmp("modem",argv[1]))
			{
				t<<"Setting Modem debug to "<<lvl<<"\n";
				cModem::get()->setDebug(lvl);
			}
			else if(!strcmp("sysmon",argv[1]))
			{
				t<<"Setting SYSMON debug to "<<lvl<<"\n";
				cSysMon::get()->setDebug(lvl);
			}
			else if(!strcmp("cfg",argv[1]))
			{
				t<<"Setting Config debug to "<<lvl<<"\n";
				cConfig::get()->setDebug(lvl);
			}
			else if(!strcmp("log",argv[1]))
			{
				t<<"Setting Logger debug to "<<lvl<<"\n";
				cLog::get()->setDebug(lvl);
			}
		}
		else
		{
			t<<"See Help on how to use it \n";
		}
	}

}

void System::reset(cTerm & t, int argc, char *argv[])
{
	diag_printf("System will now RESET\n");
	cyg_uint32 reg32;
	//HAL_READ_UINT32(0xE000ED00 + 0x0C, reg32); //SCB_AIRCR, SYSRESETREQ
	reg32 = (0x5FA << 16) | (1 << 2);
	HAL_WRITE_UINT32(0xE000ED00 + 0x0C, reg32);
}



void System::flashCmd(cTerm & t,int argc,char *argv[])
{
	if (!strcmp(argv[0],"flrd"))
	{
		if (argc > 2)
		{
			cyg_uint32 addr = (cyg_uint32)strtoul(argv[1],NULL,16);
			cyg_uint32 num = (cyg_uint32)strtoul(argv[2],NULL,16);

			cyg_uint8 buff[num];

			cyg_bool success = SpiFlash::get()->ReadSpi(addr, buff, num);
			t << t.format("Reading: 0x%08X\n", addr);

			if (success == true)
			{
				t.dbg_printf(green,"Read %d bytes from serial flash at address 0x%08X\n", num, addr);
				for (cyg_uint32 i = 0 ; i < num ; i++)
				{
					diag_printf(" 0x%02X", buff[i]);

				}
				diag_printf("\n");
			}
			else
			{
				t.dbg_printf(red,"Error reading from serial flash!\n");
			}
		}
		else
		{
			t<<"You need to supply an address and number of bytes to read\n";
		}
	}
	else if (!strcmp(argv[0],"flwr"))
	{
		if (argc > 3)
		{
			cyg_uint32 addr = (cyg_uint32)strtoul(argv[1],NULL,16);
			cyg_uint32 num = (cyg_uint32)strtoul(argv[2],NULL,16);
			cyg_uint8 val = (cyg_uint8)strtoul(argv[3],NULL,16);

			cyg_uint8 buff[num];

			memset(buff,val,num);


			cyg_bool success = SpiFlash::get()->WriteSpi(addr, buff, num);

			if (success == true)
			{
				t.dbg_printf(green,"Wrote %d bytes of 0x%02X to serial flash at address 0x%08X\n", num, val, addr);
			}
			else
			{
				t.dbg_printf(red,"Error writing to serial flash!\n");
			}
		}
		else
		{
			t<<"You need to supply an address, number of bytes and value to write\n";
		}
	}
	else if (!strcmp(argv[0],"fle"))
	{
		if (argc > 1)
		{
			cyg_uint32 addr = (cyg_uint32)strtoul(argv[1],NULL,16);

			cyg_bool success = SpiFlash::get()->flash_erase(addr);

			if (success == true)
			{
				t.dbg_printf(green,"Erased serial flash sector at address 0x%08X\n", addr);
			}
			else
			{
				t.dbg_printf(red,"Error erasing serial flash!\n");
			}
		}
		else
		{
			t<<"You need to supply an address in a sector to erase\n";
		}
	}
}

TermCMD::TermCMD()
{
}

void TermCMD::process(cTerm & term, int argc,char * argv[])
{
	cmd_table_t* t_ptr = NULL;

	int k = 0;
	do
	{
		t_ptr = &mCmdTable[k++];
		if(!t_ptr->cmd)
			break;

		//Special AT commands for modem
		if(!strncmp(argv[0],"AT",2))
		{
			modem::ATcmd(term, argc, argv);
			return;
		}

		if(t_ptr->f && !strcmp(argv[0],t_ptr->cmd))
		{
			t_ptr->f(term, argc, argv);
			return;
		}
	}while(t_ptr->cmd);

	if(cDebug::VT100flag)
		term<<cDebug::VT100_RED;
	term<<"Unknown Command \'"<<argv[0]<<"\'. Type help for a list of commands\n";
	if(cDebug::VT100flag)
		term<<cDebug::VT100_NONE;
}

void TermCMD::help(cTerm & t,int argc,char *argv[])
{
	t.dbg_printf(green,"TermCMD commands:\n");

	cmd_table_t* t_ptr = NULL;
	char txt[16];

		int k = 0;
		do
		{
			t_ptr = &mCmdTable[k++];
			if(!t_ptr->cmd)
				break;

			if(t_ptr->f)
			{
				sprintf(txt,"%s %s", t_ptr->cmd, t_ptr->argDesc);
				t<<"  "<<t.format("%-10s - ",txt)<<t.format("%s\n",t_ptr->desc);
			}
			else
			{
				//this is a caption
				t.dbg_printf(blue, "%s\n", t_ptr->cmd);
			}

		}while(t_ptr->cmd);

}

