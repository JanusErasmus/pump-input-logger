#ifndef _DEBUG_H_
#define _DEBUG_H_

enum sVT100command
{
	clearScreen,
	red,
	green,
	blue,
	yellow,
	cyan
};

class cDebug
{
public:
	//VT100 definitions
	static bool VT100flag;
	static char VT100_ID[24];
	static char VT100_HOME[16];
	static char VT100_RED[16];
	static char VT100_GREEN[16];
	static char VT100_BLUE[16];
	static char VT100_CYAN[16];
	static char VT100_YELLOW[16];
	static char VT100_BLACKBACK[16];
	static char VT100_GREENBACK[16];
	static char VT100_NONE[16];
	static char VT100_UNDERLN[16];

protected:

	virtual ~cDebug();

   char str[256];
   int mDebugLevel;
   void dbg_printf(int lvl,const char * fmt,...);
   void dbg_dump_buf(int lvl, void * b,int len);


public:
   cDebug();
   virtual void setDebug(int lvl);

   //VT100 commands
   	void dbg_printf(sVT100command command,const char * fmt,...);

};

#endif //Include Guard
