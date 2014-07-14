#ifndef TERMCMD_H_
#define TERMCMD_H_
#include <cyg/hal/hal_tables.h>
#include <cyg/kernel/kapi.h>
#include "debug.h"
#include "term.h"

class TermCMD
{

	typedef void (*func_t)(cTerm & t,int argc,char *argv[]);

	    typedef struct
	    {
	    	char* cmd;
	    	char* argDesc;
	    	char* desc;
	        func_t f;
	    } cmd_table_t;
	    static cmd_table_t mCmdTable[];

public:
	TermCMD();
	static void process(cTerm & term, int argc,char * argv[]);
	static void help(cTerm & t,int argc,char *argv[]);
};

class systemMon
{
public:
	static void sync(cTerm & term, int argc,char * argv[]);
	static void syncDone(cTerm & term, int argc,char * argv[]);
	static void stat(cTerm & term, int argc,char * argv[]);
	static void setPeriod(cTerm & term, int argc,char * argv[]);
	static void setPowerStat(cTerm & term, int argc,char * argv[]);
};


class logEvent
{
public:
	static void log(cTerm & term, int argc,char * argv[]);
	static void logDisc(cTerm & term, int argc,char * argv[]);
	static void ack(cTerm & term, int argc,char * argv[]);
	static void showAll(cTerm & term, int argc,char * argv[]);
};

class MCP
{
public:
	static void sync(cTerm & term, int argc,char * argv[]);
	static void set(cTerm & term, int argc,char * argv[]);
	static void powerChange(cTerm & term, int argc,char * argv[]);
};

class inputs
{
public:
	static void discrete(cTerm & term, int argc,char * argv[]);
	static void analog(cTerm & term, int argc,char * argv[]);
	static void showM(cTerm & term, int argc,char * argv[]);
	static void incHobbs(cTerm & term, int argc,char * argv[]);
	static void showHobbs(cTerm & term, int argc,char * argv[]);
};

class modem
{
public:
	static void ATcmd(cTerm & term, int argc,char * argv[]);
	static void cmd(cTerm & term, int argc,char * argv[]);
	static void status(cTerm & term, int argc,char * argv[]);
	static void reset(cTerm & term, int argc,char * argv[]);
	static void fixBaud(cTerm & term, int argc,char * argv[]);
};

class System : cDebug
{
public:
	static void handle(cTerm & t,int argc,char *argv[]);
	static void flashCmd(cTerm & t,int argc,char *argv[]);
	static void setDebugLvl(cTerm & t,int argc,char *argv[]);

	static void nvmBuff(cTerm & t,int argc,char *argv[]);
	static void config(cTerm & t,int argc,char *argv[]);
	static void reset(cTerm & t,int argc,char *argv[]);

	static void setUpperLimit(cTerm & t,int argc,char *argv[]);
	static void setLowerLimit(cTerm & t,int argc,char *argv[]);
};

#endif /* TERMCMD_H_ */

