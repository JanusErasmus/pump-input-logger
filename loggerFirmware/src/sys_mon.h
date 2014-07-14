#ifndef SYSMON_H_
#define SYSMON_H_
#include <cyg/kernel/diag.h>
#include <cyg/kernel/kapi.h>
#include <time.h>

#include "definitions.h"
#include "modem.h"
#include "log.h"
#include "debug.h"
#include "rmmdict.h"
#include "watchdog.h"

class cSysMon : public cDebug
{
	cSysMon();
	static cSysMon* _instance;

	cyg_uint8 mTXbuff[MODEM_BUFF_SIZE];
	cyg_uint8 mRXbuff[MODEM_BUFF_SIZE];
	cyg_uint16 mRXlen;
	bool replied;
	cyg_mutex_t mBuffMutex;
	cyg_mutex_t mWaitMutex;
	cyg_cond_t mWaitCond;

	cyg_mutex_t mEventMutex;
	cyg_cond_t mEventCond;
	cEvent mEvent;

	cyg_uint8 mStack[SYSMON_STACK_SIZE];
	cyg_thread mThread;
	cyg_handle_t mThreadHandle;
	static void sys_thread_func(cyg_addrword_t arg);

	cyg_mutex_t mSessionMutex;

	time_t mLastSession;
	cyg_uint32 mUploadFailCnt;

	bool mPowerStat;

	wdKicker* mWatchDog;

	void monitor();
	void checkInputState();
	void logInputStateNow();

	cyg_bool uploadLogs(cyg_uint32 retryCount);
	cyg_uint8 doSession();
	cyg_uint8 attachModem();
	cyg_uint8 doTransfer();

	bool insertCfg(cyg_uint16 &idx, cyg_uint8* buff,  cyg_uint16 buffLen);
	bool insertTime(cyg_uint8 port, cyg_uint16 &idx, cyg_uint8* buff,  cyg_uint16 buffLen);
	bool insertEvent(cEvent* e, cyg_uint16 &idx, cyg_uint8* buff,  cyg_uint16 buffLen);
	bool insertPwrChange(cyg_uint16 &idx, cyg_uint8* buff,  cyg_uint16 buffLen);
	bool waitReply(cyg_uint8* buff, cyg_uint16 len);
	bool validFrame(cyg_uint8* buff, cyg_uint16 len);
	bool handleMessage(uKMsg* m);
	bool isMyMessage(uKMsg* m);
	bool handleSetTime(uKMsg* m);
	bool handleSetHobbs(uKMsg* m);
	bool handleSetCFG(uKMsg* m);

    void printMessage(uKMsg* m);
    void shutDownModem();


public:
	static void init();
	static cSysMon* get();

	void setPowerStat(bool stat);
	void setUploadDone(bool stat = 1);
	void logEvent(cEvent *e);

	void showStat();
	time_t getLastSync();

	void dataReceived(cyg_uint8* buff, cyg_uint16 len);
	virtual ~cSysMon();
};

#endif /* SYSMON_H_ */
