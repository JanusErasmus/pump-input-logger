#include <cyg/kernel/diag.h>
#include <cyg/io/ttyio.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "term.h"
#include "TermCMD.h"
#include "utils.h"

cTerm * cTerm::__instance = NULL;

cTerm::cTerm(char * dev,cyg_uint32 b_size,const char * const prompt_str)
{
    Cyg_ErrNo err;
    cyg_tty_info_t tty_info;

    cyg_uint32 len = strlen(dev)+1;
    mDev = new char[len];
    strcpy(mDev,dev);

    mBuffSize = b_size;
    mRxBuff = new char[mBuffSize];
    mRxIdx = 0;

    mPrompt = prompt_str;

    err = cyg_io_lookup(mDev,&mDevHandle);

    diag_printf("TERM: Lookup error %s \n",strerror(-err));

    len = sizeof(tty_info);
    cyg_io_get_config(mDevHandle,
                      CYG_IO_GET_CONFIG_TTY_INFO,
                      &tty_info,
                      &len);

    //diag_printf("TERM: TTY in_flags = 0x%08X, out_flags = 0x%08X\n",tty_info.tty_in_flags,tty_info.tty_out_flags);

    tty_info.tty_in_flags = (CYG_TTY_IN_FLAGS_CR | CYG_TTY_IN_FLAGS_ECHO);

    cyg_io_set_config(mDevHandle,
                      CYG_IO_SET_CONFIG_TTY_INFO,
                      &tty_info,
                      &len);

    banner();
    prompt();

    cyg_thread_create(TERM_PRIOR,
                      cTerm::term_thread_func,
                      (cyg_addrword_t)this,
                      (char *)"Terminal",
                      mStack,
                      TERM_STACK_SIZE,
                      &mThreadHandle,
                      &mThread);

    cyg_thread_resume(mThreadHandle);

}


cTerm::~cTerm()
{
    delete [] mDev;
}

void cTerm::init(char * dev,cyg_uint32 b_size,const char * const prompt_str)
{
    if (__instance == NULL)
    {
        __instance = new cTerm(dev,b_size,prompt_str);
    }
}

void cTerm::term_thread_func(cyg_addrword_t arg)
{
    cTerm * t = (cTerm *)arg;




//----------------------------DETERMINE VT100 ----------------------

    if(t->isVT100())
    {
    	diag_printf("VT100 commands on\n");
    	VT100flag = true;
    }
    else
    {
    	diag_printf("VT100 commands off\n");
    }

//--------------------------------------------------

    for (;;)
    {
        t->run();
    }
}

bool cTerm::isVT100()
{
	    cyg_uint8 len = strlen(VT100_ID);
	    cyg_io_write(mDevHandle,VT100_ID,(cyg_uint32*)&len);
	    cyg_thread_delay(10);//Wait for terminal to reply
	    len = 1;
	    memset(mRxBuff,0,10);
	    cyg_io_read(mDevHandle,mRxBuff,(cyg_uint32*)&len);
	    if(len > 0)
	    {
	    	//diag_dump_buf(mRxBuff,len);
	    	if(mRxBuff[0] == 0x1B || mRxBuff[0] == 0xA3)
	    	{
	    		return true;
	    	}
	    }

	    return false;
}

void cTerm::run(void)
{
    mRxIdx = mBuffSize;
    cyg_io_read(mDevHandle,mRxBuff,&mRxIdx);

    mRxBuff[mRxIdx-1] = 0;



    if ( mRxIdx >= 1 )
    {
    	process_command_line();

    }
    prompt();
}

cTerm& cTerm::operator <<(const char *str)
{
    cyg_uint32 len = strlen(str);

    cyg_io_write(mDevHandle,str,&len);

    return(*this);
}
cTerm& cTerm::operator <<(int i)
{
    char str[20];
    sprintf(str,"%d",i);
    cyg_uint32 len = strlen(str);

    cyg_io_write(mDevHandle,str,&len);

    return(*this);
}

cTerm& cTerm::operator <<(unsigned char c)
{
    char str[6];
    sprintf(str,"0x%02X",c);
    cyg_uint32 len = strlen(str);

    cyg_io_write(mDevHandle,str,&len);

    return(*this);
}

cTerm& cTerm::operator <<(unsigned int i)
{
    char str[12];
    sprintf(str,"0x%08X",i);
    cyg_uint32 len = strlen(str);

    cyg_io_write(mDevHandle,str,&len);

    return(*this);
}
cTerm& cTerm::operator <<(void * p)
{
    *this<<(unsigned int)p;
    return(*this);
}


char * cTerm::format(const char *f,...)
{
    va_list vl;
    va_start(vl,f);
    vsprintf(&mRxBuff[0],f,vl);
    va_end(vl);

    return &mRxBuff[0];
}

void cTerm::banner()
{
    *this<<"Terminal started on device \""<<mDev<<"\"\n";
}

void cTerm::prompt()
{
	if(VT100flag)
		*this<<VT100_BLUE;
	*this<<"\n"<<mPrompt;
	if(VT100flag)
		*this<<VT100_NONE;
}


void cTerm::process_command_line(void)
{

    char *argv[20];
    int argc = 20;

    util_parse_params(mRxBuff,argv,argc,' ',' ');

    if ( argc )
    {
        dispatch_command_line(argc,argv);
    }

}

void cTerm::dispatch_command_line(int argc,char * argv[])
{
	TermCMD::process(*this,argc,argv);
}

