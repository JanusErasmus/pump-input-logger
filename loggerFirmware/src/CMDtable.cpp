#include <cyg/kernel/diag.h>

#include "TermCMD.h"
#include "stm32cpu.h"
#include "version.h"
#include "input_port.h"
#include "modem.h"
#include "nvm.h"
#include "sys_mon.h"
#include "log.h"
#include "MCP_rtc.h"
#include "picaxe_lcd.h"

TermCMD::cmd_table_t TermCMD::mCmdTable [] =
{
	{"SYSTEM"	,0,0,0},
    {"h"	, "",			"Show this help info", TermCMD::help},
    {"help"	, "",			"Show this help info", TermCMD::help},
    {"dt"	, "",			"Dump Thread Info", System::threadInfo},
    {"ram"	, "",			"Show RAM usage", System::ramUsage},
    {"pstat", "",			"Processor Power Status", stm32cpu::pStatus},
    {"reset", "",			"Reset the processor", System::reset},
    {"time"	, "",			"Current system time", System::eCOStime},
    {"cls"	, "",			"Clear the screen", System::clear},

    {"VERSION & SERIAL"	,0,0,0},
    {"debug", "<mod> <lvl>","Set the Debug level for a module, 0 disables debug output", System::setDebugLvl},
    {"ver"	, "",			"Show the build and commit dates for this binary", System::version},
    {"sn"	, "",			"Set 64bit adress LSB first", System::serial},

    {"FLASH"	,0,0,0},
    {"flrd"	, "<addr> <num>","Read num bytes from serial flash at addr", SpiFlash::readCmd},
    {"flwr"	, "<addr> <num> <val>", "Write num bytes of val to serial flash at addr", SpiFlash::writeCmd},
    {"fle"	, "<addr>",		"Erase serial flash sector containing addr", SpiFlash::eraseCmd},

    {"IO"	,0,0,0},
    {"i"	,"",			"Displays the port input states", cInput::showInputs},

    {"NVM"	,0,0,0},
    {"cfg","<param to change>","Configures the dial values", cNVM::config},

    {"MODEM"	,0,0,0},
    {"AT****", "",			"Send modem cmd (CAPITALIZED)", cModem::ATcmd},
    {"modemStat", "",		"Get modem status", cModem::debug},
    {"modemUp", "",		"Update modem status", cModem::debug},
    {"pb", "",		"list modem phonebook entries", cModem::debug},
    {"sms", "<number>",		"send test SMS", cModem::debug},
    {"rsms", "<number>",		"Show all SMS's", cModem::debug},
    {"bal", "<number>",		"SIM balance", cModem::debug},

    {"SYSMON"	,0,0,0},
    {"pwrModem", "<stat>",	"Set SYSMON pwrStat and Press modem PWR_KEY", cSysMon::setPowerStat},
    {"nav", "<u,d,l,r>",	"Navigate Display", cSysMon::navigate},

    {"Logs"	,0,0,0},
    {"show", "",			"Show all logs", cLog::logDebug},
    {"log", "",				"Log a event", cLog::logDebug},

    {"RTC TIME"	,0,0,0},
    {"setRTC", "<yyyy mm dd HH MM SS>","Set MCP RTC", cRTC::set},

    {"PICAXE serial-LCD",0,0,0},
    {"lcd", "",		"Debug LCD", cPICAXEserialLCD::debugCMD},

    {0,0,0,0},
};


void System::version(cTerm & t,int argc,char *argv[])
{
	diag_printf("Software Version  : %d.%d.%d\n",(VERSION_NUM & 0xFF0000)>>16,(VERSION_NUM & 0xFF00)>>8,(VERSION_NUM & 0xFF));
		t<<"Build Timestamp   : "<<BUILD_DATE<<"\n";
}
