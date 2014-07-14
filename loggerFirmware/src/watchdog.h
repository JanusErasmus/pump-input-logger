#ifndef WD_H_
#define WD_H_
#include <cyg/kernel/kapi.h>
#include <cyg/io/io.h>

#include "debug.h"
#include "definitions.h"

#define KICK_COUNT 10

class wdKicker
{
	cyg_int32 mTimeOut;
	cyg_int32 mCount;
	bool mEnabled;

public:
	wdKicker(cyg_int32 time);
	void sleep();
	void reset();
	bool isSleeping();

	void enable();
	void disable();

	virtual ~wdKicker();
};

class cWatchDog : public cDebug
{
	static cWatchDog* _instance;

    cyg_io_handle_t mSerCMDHandle;
    wdKicker* mKickers[KICK_COUNT];
    cyg_uint8 mKickCnt;


    cyg_uint8 mStack[WD_STACK_SIZE];
    cyg_thread mThread;
    cyg_handle_t mThreadHandle;
    static void wd_thread_func(cyg_addrword_t arg);
    void run();

    cWatchDog();

public:
	static void init();
	static cWatchDog* get();

	bool registerKicker(wdKicker* kicker);

	virtual ~cWatchDog();
};


#endif /* WATCHDOG_H_ */
