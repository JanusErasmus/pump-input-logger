#ifndef TERMCMD_H_
#define TERMCMD_H_
#include <cyg/hal/hal_tables.h>
#include <cyg/kernel/kapi.h>
#include <cyg/kernel/diag.h>

#include "debug.h"
#include "term.h"

class TermCMD : cDebug
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

class System : cDebug
{
public:
	static void handle(cTerm & t,int argc,char *argv[]);
	static void setDebugLvl(cTerm & t,int argc,char *argv[]);

	static void nvmBuff(cTerm & t,int argc,char *argv[]);
	static void threadInfo(cTerm & t,int argc,char *argv[]);
	static void ramUsage(cTerm & t,int argc,char *argv[]);
	static void version(cTerm & t,int argc,char *argv[]);
	static void serial(cTerm & t,int argc,char *argv[]);

	static void eCOStime(cTerm & t,int argc,char *argv[]);
	static void clear(cTerm & t,int argc,char *argv[]);
	static void reset(cTerm & t, int argc, char *argv[]);

};

class SpiFlash
{
public:
    static void readCmd(cTerm & t,int argc,char *argv[]);
    static void writeCmd(cTerm & t,int argc,char *argv[]);
    static void eraseCmd(cTerm & t,int argc,char *argv[]);
};


#endif /* TERMCMD_H_ */
