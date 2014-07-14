#ifndef ONTIME_H_
#define ONTIME_H_
#include <cyg/kernel/kapi.h>
#include <time.h>

#include "definitions.h"

/** map this variable to a flash sector,
 * and write sequencialy to the page until end of page is reached,
 * and start over again.
*/
class cHobbsTimer
{
	static cHobbsTimer* _instance;
	cyg_uint32 mSector;
	cyg_uint32 mReadAddr;

	struct sOnTime
	{
		cyg_uint32 total;
		cyg_uint32 input0;
		cyg_uint32 input1;
		cyg_uint8 mCrc;
	}__attribute__((packed));

    cyg_mutex_t mOnMutex;
    sOnTime mOnTime;
    cHobbsTimer(cyg_uint32 sectorStart, cyg_uint32 period);
    static cyg_uint8 mPerInc;
    cyg_mutex_t mWaitMutex;
    cyg_cond_t mWaitCond;
    cyg_uint8 mStack[HOBB_STACK_SIZE];
    cyg_thread mThread;
    cyg_handle_t mThreadHandle;
    static void hobb_thread_func(cyg_addrword_t arg);
    cyg_uint8 mAlarmPeriod;
    cyg_alarm mAlarm0, mAlarm1;
    time_t mOnTime0, mOnTime1;
    cyg_handle_t mAlarmHandle0, mAlarmHandle1;
    static void alarm_handler(cyg_handle_t alarm, cyg_addrword_t data);
    void setAddress();

    bool checkSpace();

public:
	static void init(cyg_uint32 sectorStart, cyg_uint32 period);
	static cHobbsTimer* get();
    void start();

	void setPortState(cyg_uint8 input, bool state);

	void incTime(cyg_uint8 input, cyg_uint32 time);
	void setTime(cyg_uint8 input, cyg_uint32 time);
	cyg_uint32 getTime(cyg_uint8 input);
	void showTimes();

	virtual ~cHobbsTimer();
};

#endif /* ONTIME_H_ */
