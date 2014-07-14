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

#define POWER_IN_PORT 4
#define ALARM_SET_PORT 99

class cSysMon : public cDebug, public cAlarmCallback, public cActionQueue
{
public:
	enum e_sysmon_action
	{
		sysmonActionTest,
		sysmonActionPowerLoss,
		sysmonActionPowerRestored,
		sysmonActionAlarm,
		sysmonActionSMS
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

	cyg_bool mPowerFlag;

	time_t mAlarmTimer;
	cyg_bool mAlarmEnabled;
	cyg_bool mAlarmAck;
	cyg_uint8 mAlarmAckCnt;
	cyg_bool mAlarmDisEnable;
	cyg_uint8 mWalkOutTime;

	cyg_bool mAlarmFrameState;

	cyg_bool mTestMissedCallFlag;

	cyg_uint32 mUploadFailCnt;


	wdKicker* mWatchDog;

	cyg_bool monitor();
	cyg_bool handleAction(cyg_addrword_t action);
	cyg_bool handleEvent(s_event* evt);

	void armDisarm();
	void updateAlarmState();
	void arm();
	void disarm();

	cLED::eLEDstatus registerModem();
	cLED::eLEDstatus checkSIM();

	bool sendPowerSMS(cyg_bool state);
	bool placeMissedCall();

	void handleSMSlist();
	void handleSMScommand(cMdmReadSMS::sSMS * sms);

public:
	static void init();
	static cSysMon* get();

	void showStat();
	time_t getLastSync();

	void acknowledgeAlarm();

	static void setPowerStat(cTerm & term, int argc,char * argv[]);

	virtual ~cSysMon();
};

#endif /* SYSMON_H_ */
