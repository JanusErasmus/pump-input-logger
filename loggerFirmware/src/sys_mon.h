#ifndef SYSMON_H_
#define SYSMON_H_
#include <cyg/kernel/diag.h>
#include <cyg/kernel/kapi.h>
#include <time.h>

#include "definitions.h"
#include "action_queue.h"
#include "modem.h"
#include "debug.h"
#include "watchdog.h"
#include "led.h"
#include "standby_menu.h"

class cSysMon : public cDebug, public cActionQueue
{
public:
	enum e_sysmon_action
	{
		sysmonAction,
	};

private:

	cSysMon();
	static cSysMon* _instance;

	cyg_uint8 mTXbuff[MODEM_BUFF_SIZE];
	cyg_uint8 mRXbuff[MODEM_BUFF_SIZE];
	cyg_uint16 mRXlen;
	bool replied;
	cyg_mutex_t mBuffMutex;
	cyg_mutex_t mWaitMutex;
	cyg_cond_t mWaitCond;

	cyg_uint8 mStack[SYSMON_STACK_SIZE];
	cyg_thread mThread;
	cyg_handle_t mThreadHandle;
	static void sys_thread_func(cyg_addrword_t arg);

	cyg_mutex_t mMonitorMutex;

	cLCDmenu * mMenu;

	wdKicker* mWatchDog;

	cyg_bool monitor();
	cyg_bool handleAction(cyg_addrword_t action);
	cyg_bool handleEvent(s_event* evt);

	cLED::eLEDstatus registerModem();
	cLED::eLEDstatus checkSIM();

	bool sendPowerSMS(cyg_bool state);
	bool placeMissedCall();

	void handleSMSlist();
	void handleSMScommand(cMdmReadSMS::sSMS * sms);

public:
	static void init();
	static cSysMon* get();

	static void setPowerStat(cTerm & term, int argc,char * argv[]);
	static void navigate(cTerm & term, int argc,char * argv[]);

	virtual ~cSysMon();
};

#endif /* SYSMON_H_ */
