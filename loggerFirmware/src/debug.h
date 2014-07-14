#ifndef _DEBUG_H_
#define _DEBUG_H_

//#define VT100_OFF

#ifndef VT100_OFF
#define COLOR(__c,__x)	"\x1b[3"#__c"m"__x"\x1b[0m"
#define COLOR_BOLD(__c,__x)	"\x1b[3"#__c";1m"__x"\x1b[0m"
#define UNDERLINE(__x) "\x1b[4m"__x"\x1b[0m"
#else
#define COLOR(__c,__x)	__x
#define COLOR_BOLD(__c,__x) __x
#define UNDERLINE(__x) __x
#endif
#define RED(__x)		COLOR(1,__x)
#define GREEN(__x)		COLOR(2,__x)
#define YELLOW(__x)		COLOR(3,__x)
#define BLUE(__x)		COLOR(4,__x)
#define MAGENTA(__x)	COLOR(5,__x)
#define CYAN(__x)		COLOR(6,__x)
#define RED_B(__x)		COLOR_BOLD(1,__x)
#define GREEN_B(__x)		COLOR_BOLD(2,__x)
#define YELLOW_B(__x)		COLOR_BOLD(3,__x)
#define BLUE_B(__x)		COLOR_BOLD(4,__x)
#define MAGENTA_B(__x)	COLOR_BOLD(5,__x)
#define CYAN_B(__x)		COLOR_BOLD(6,__x)


class cDebug
{
protected:

	virtual ~cDebug();

   char str[256];
   int mDebugLevel;
   void dbg_printf(int lvl,const char * fmt,...);
   void dbg_dump_buf(int lvl, void * b,int len);


public:
   cDebug();
   virtual void setDebug(int lvl);


};

#endif //Include Guard
