#include <stdio.h>

#include "sys_mon.h"

#include "temp_mon.h"
#include "MCP_rtc.h"
#include "hobbs_timer.h"
#include "led.h"
#include "nvm.h"
#include "crc.h"
#include "pwr_mon.h"

cSysMon* cSysMon::_instance = 0;

void cSysMon::init()
{
	if(!_instance)
	{
		_instance = new cSysMon();
	}
}


cSysMon* cSysMon::get()
{
	return _instance;
}

cSysMon::cSysMon()
{
	mRXlen = 0;
	mPowerStat = false;
	replied = false;
	
	mLastSession = 0;
	mUploadFailCnt = 0;

	mWatchDog = new wdKicker(300);
	diag_printf("WATCHDOG %p for SYSMON\n", mWatchDog);

	cyg_mutex_init(&mSessionMutex);

	cyg_mutex_init(&mBuffMutex);
	cyg_mutex_init(&mWaitMutex);
	cyg_cond_init(&mWaitCond, &mWaitMutex);

	cyg_mutex_init(&mEventMutex);
	cyg_cond_init(&mEventCond, &mEventMutex);

	cyg_thread_create(SYSMON_PRIOR,
			cSysMon::sys_thread_func,
			(cyg_addrword_t)this,
			(char *)"SysMon",
			mStack,
			SYSMON_STACK_SIZE,
			&mThreadHandle,
			&mThread);

	cyg_thread_resume(mThreadHandle);

}

void cSysMon::sys_thread_func(cyg_addrword_t arg)
{
	cSysMon *t = (cSysMon *)arg;
	for(;;)
	{
		//do every 30s
		cyg_uint32 wait = cyg_current_time() + 15000;
		if(cyg_cond_timed_wait(&(t->mEventCond), wait))
		{
			if(t->mEvent.getType() != cEvent::EVENT_UNKNOWN)
			{
				diag_printf("Logging event\n");
				t->mEvent.showEvent();

				cLog::get()->logEvent(&(t->mEvent));

				cEvent n;
				t->mEvent = n;
			}
		}
		else
		{
			//monitor the temperatures and transmit logs at set interval
			//TODO analogue inputs NOT logged t->monitor();
		}
		t->mWatchDog->reset();


		//Do nothing when the unit has not been assigned a box number
		if(!cNVM::get()->getBoxSerial())
			continue;
	}
}



void cSysMon::monitor()
{
	static time_t mLastLog = 0;
	static cyg_bool criticalUploadFlag = false;
	
	time_t now = cRTC::get()->timeNow();

	//sample temperatures and power source every hour
	if(now > (mLastLog + 3600))
	{
		diag_printf("Doing hourly maintenance\n");
		mLastLog = now;
		logInputStateNow();
		cTempMon::get()->logSamplesNow();
	}

	//sample input states if it has changed
	checkInputState();

	//log temperatures only once
	static cyg_bool logTempFlag = false;

	//sample temperatures and if they differ with more than the set range log them
	if(cTempMon::get()->checkTemps())
	{
		criticalUploadFlag = true; //if a temperature is critical upload logs NOW
		logTempFlag = false;
	}

//	TODO Upload of logs disabled
//	//upload logs if any of the temperatures' state change
//	if(criticalUploadFlag)
//	{
//		diag_printf("Temperature out of bounds has changed, uploading logs in %ds\n", (600/30 - (mUploadFailCnt + 5))*30);
//
//		//when temperatures changed log them now
//		if(!logTempFlag)
//		{
//			logTempFlag = true;
//			cTempMon::get()->logSamplesNow();
//		}
//
//		//if upload fails, it will retry in 600s (10min)
//		if(uploadLogs(600))
//		{
//			criticalUploadFlag = false;
//		}
//	}
//	else if( now == 0 || (unsigned int)now > (mLastSession + cNVM::get()->getLogPeriod())) //upload the logs every log period
//	{
//		//if upload fails, it will retry in 3600s (1hour)
//		uploadLogs(3600);
//	}
}

void cSysMon::logEvent(cEvent *e)
{
	memcpy(&mEvent, e, sizeof(cEvent));
	delete e;

	cyg_cond_signal(&mEventCond);

}
//only log state if it has changed
void cSysMon::checkInputState()
{
	static cyg_uint8 DieselStat = 0;
	static cyg_uint8 ElectricStat = 0;

	//log event if input changed
	cyg_uint8 currState = cPwrMon::get()->getPinStat(0);
	if(DieselStat != currState)
	{
		DieselStat = currState;
		cEvent e(0, currState, cRTC::get()->timeNow());
		e.showEvent();
		cLog::get()->logEvent(&e);
	}

	//log event if input changed
	currState = cPwrMon::get()->getPinStat(1);
	if(ElectricStat != currState)
	{
		ElectricStat = currState;
		cEvent e(1, currState, cRTC::get()->timeNow());
		e.showEvent();
		cLog::get()->logEvent(&e);
	}
}

void cSysMon::logInputStateNow()
{
	diag_printf("Logging power states\n");
	// log diesel input state
	{
		cyg_uint8 currState = cPwrMon::get()->getPinStat(0);
		cEvent e(0, currState, cRTC::get()->timeNow());
		e.showEvent();
		cLog::get()->logEvent(&e);
	}
	//log electric input state
	{
		cyg_uint8 currState = cPwrMon::get()->getPinStat(1);
		cEvent e(1, currState, cRTC::get()->timeNow());
		e.showEvent();
		cLog::get()->logEvent(&e);
	}
}

cyg_bool cSysMon::uploadLogs(cyg_uint32 retryCount)
{
	cyg_bool uploadStatus = false;

	if(mUploadFailCnt < 5)
	{
		cyg_mutex_lock(&mSessionMutex);

		diag_printf("MONITOR: Last session was %s", ctime(&mLastSession));
		cyg_uint8 err = doSession();
		if(err)
		{
			cLED::get()->indicate((cLED::eLEDstatus)err);

			mUploadFailCnt++;
			dbg_printf(red, "SESSION: Unsuccessful transfer\n");
		}
		else
		{
			cLED::get()->indicate(cLED::idle);
			mUploadFailCnt = 0;
			uploadStatus = true;

			dbg_printf(green, "SESSION: Transfer Successful\n");

			//when successful, do again next session period
			mLastSession = cRTC::get()->timeNow();
		}
		cyg_mutex_unlock(&mSessionMutex);
	}
	else
	{
		mUploadFailCnt++;
	}

	//when failed repeatedly, switch off modem and retry after a longer time
	if(mUploadFailCnt == 5)
	{
		dbg_printf(red, "SESSION: Shutting down modem\n");
		shutDownModem();
	}

	//retry to transfer logs when transfer failed every retryCount seconds
	if(mUploadFailCnt > (retryCount/30)) //monitor is called every 30seconds so this will restart after a power down
	{
		dbg_printf(green, "SESSION: Transfer retry after wait\n");
		mUploadFailCnt = 0;

		static cyg_uint8 retryFailFlag = 0;
		if(retryFailFlag++ > 2)
		{
			diag_printf("Transfer fails, System will restart now\n");
			cyg_thread_delay(100);
			cyg_uint32 reg32;
			//HAL_READ_UINT32(0xE000ED00 + 0x0C, reg32); //SCB_AIRCR, SYSRESETREQ
			reg32 = (0x5FA << 16) | (1 << 2);
			HAL_WRITE_UINT32(0xE000ED00 + 0x0C, reg32);
		}
	}

	return uploadStatus;
}

void cSysMon::shutDownModem()
{
    cModem::get()->shutIP();
    cModem::get()->IPstatus();
    if(!mPowerStat)
    {
        cModem::get()->power(0);
    }
}

cyg_uint8 cSysMon::doSession()
{
	cyg_uint8 err;	

	//try setup Modem
	err = attachModem();
	if(err)
		return err;

	diag_printf("SESSION: Modem Linked,\n");

	//modem now connected, transfer logs to server
	err = doTransfer();
	if(err)
		return err;

    //when successful transfer powerModem down
    shutDownModem();

	return 0;
}

cyg_uint8 cSysMon::attachModem()
{
	cLED::get()->indicate(cLED::connecting);

	cModem::eModemStat stat = cModem::get()->getStatus();

	//when modem is off, power it up
	if(stat == cModem::Off)
	{
		cModem::get()->power(1);
		diag_printf("SESSION: Powering modem\n");
		//wait for modem to initialise
		cyg_thread_delay(1000);

		stat = cModem::get()->getStatus();
		if(stat == cModem::Off)
		{
			//restart yourself if the modem seems not to power up
			static cyg_uint8 mdmPwrFlag = 0;
			if(mdmPwrFlag++ > 2)
			{
				diag_printf("Modem does not power Up, System will restart now\n");
				cyg_thread_delay(100);
				cyg_uint32 reg32;
				//HAL_READ_UINT32(0xE000ED00 + 0x0C, reg32); //SCB_AIRCR, SYSRESETREQ
				reg32 = (0x5FA << 16) | (1 << 2);
				HAL_WRITE_UINT32(0xE000ED00 + 0x0C, reg32);

				return cLED::couldNotPwrModem;
			}
		}
	}

	cModem::get()->setFixedBaud();
	cModem::get()->setEcho(0);

	do
	{
		//check SIM status
		switch(stat)
		{
			case cModem::needSIM:
				diag_printf("SESSION: Insert SIM - Powering down modem\n");
				cModem::get()->power(0);
				return cLED::SIMerr;

			case cModem::needPUK:
			{
				if(!cNVM::get()->getSimPukFlag() && cModem::get()->insertPUK(cNVM::get()->getSimPuk(), cNVM::get()->getSimPin()))
				{
					//wait for PIN accepted
					cyg_thread_delay(500);
				}
				else
				{
					cNVM::get()->setSimPukFlag(true);
					diag_printf("SESSION: SIM blocked, PUK needed\n");
					return cLED::PUKerr;
				}
			}
				break;

			case cModem::needPIN:
				if(cModem::get()->insertPIN(cNVM::get()->getSimPin()))
				{
					//wait for PIN accepted
					cNVM::get()->setSimPukFlag(false);
					cyg_thread_delay(500);
				}
				else
				{
					diag_printf("SESSION: incorrect PIN\n");
					return cLED::PINerr;
				}
				break;

			case cModem::NetworkBusy:
			case cModem::NetworkReg:
				diag_printf("SESSION: Waiting for Network\n");
				return cLED::waitNetwork;

			case cModem::Off:
				diag_printf("SESSION: Modem is OFF\n");
				return cLED::couldNotPwrModem;
				break;

			default:
				break;

		}

		stat = cModem::get()->getStatus();

	}while(!(stat == cModem::GPRSatt || stat == cModem::Linked));
	diag_printf("SESSION: Modem attached\n");

	//link Modem to server
	if(!cModem::get()->upModemLink(cNVM::get()->getServer(), cNVM::get()->getPort(), cNVM::get()->getAPN(), cNVM::get()->getUser(), cNVM::get()->getPasswd()))
	{
		cModem::get()->shutIP();
		return cLED::LinkErr;
	}

	return 0;
}

cyg_uint8 cSysMon::doTransfer()
{
	cLED::get()->indicate(cLED::transfering);
	cEvent e;
	bool hobbsTimesFlag = true;
	static bool startUpFlag = false;

	dbg_printf(1,"Try transfer\n");
	do
	{
		cyg_uint16 len = 0;

		if(hobbsTimesFlag)
		{
			dbg_printf(1,"Insert Times\n");

			//Fill TXbuffer with hobbs timer times
			insertCfg(len, mTXbuff, MODEM_BUFF_SIZE);
			insertTime(0, len, mTXbuff, MODEM_BUFF_SIZE); //timer for port 0
			insertTime(1, len, mTXbuff, MODEM_BUFF_SIZE); //timer for port 1
			insertTime(2, len, mTXbuff, MODEM_BUFF_SIZE); //timer for port 2

			//When the system starts up for the first time, send the power Change time stamps
			if(!startUpFlag)
			{
				if(insertPwrChange(len, mTXbuff, MODEM_BUFF_SIZE))
					startUpFlag = true;
			}

			hobbsTimesFlag = false;
		}

		//Fill TXbuffer with events
		cyg_uint32 logCnt = 0;
		do
		{
			dbg_printf(1,"Insert Event\n");
			if(cLog::get()->readEvent(&e))
			{
				if(!insertEvent(&e, len, mTXbuff, MODEM_BUFF_SIZE))
					break;

				logCnt++;
			}
		}while(cLog::get()->readNext());


		//move back in log
		for(cyg_uint8 k = 0 ; k < logCnt; k++)
			cLog::get()->readPrev();

		dbg_printf(1,"Moved back in log\n");


		if(!waitReply(mTXbuff, len))
		{
			if(startUpFlag)
				startUpFlag = false;
			return cLED::noReply;
		}

		dbg_printf(1,"RX reply\n");

		cyg_mutex_lock(&mBuffMutex);
		bool stat = validFrame(mRXbuff, mRXlen);
		cyg_mutex_unlock(&mBuffMutex);

		if(!stat)
		{
			if(startUpFlag)
				startUpFlag = false;
			return cLED::invalidFrame;;
		}

		//ACK these events in log when data acknowledged by server
		for(cyg_uint8 k = 0 ; k < logCnt; k++)
		{
			cLog::get()->acknowledge();
			cLog::get()->readNext();
		}

		dbg_printf(1,"Packet sent\n");

	}while(cLog::get()->readEvent(&e));

	dbg_printf(1,"Session done\n");
	return 0;
}

bool cSysMon::validFrame(cyg_uint8* buff, cyg_uint16 len)
{
	if(len == 0)
		return false;

	dbg_printf(3, "SYSMON: RX msg: %d\n", len);
	dbg_dump_buf(3 ,buff,len);

	cyg_uint16 idx = 0;

	do
	{
		cyg_uint16 msgLen = buff[idx];

		if(msgLen == 0)
			return false;

		dbg_printf(2, "Message len %d\n", msgLen);

		idx++;
		if(cCrc::crc8(&buff[idx], msgLen + 1))
			return false;

		uKMsg m(RMMdict::MsgDict, RMMdict::TagDict, &buff[idx], msgLen);
		handleMessage(&m);

		idx += msgLen + 1;

	}while(idx < len);


	return true;
}

bool cSysMon::handleMessage(uKMsg *m)
{

	if(!m)
		return false;


	if(!isMyMessage(m))
		return false;

//--------------PRINT TAGS----------------------------------
	if(mDebugLevel >= 1)
	{
		printMessage(m);
	}
//----------------------------------------------------------

	switch(m->id())
	{
		case RMMdict::MSG_SET_TIME:
			handleSetTime(m);
			break;

		case RMMdict::MSG_SET_LOG_CNF:
			handleSetCFG(m);
			break;

		case RMMdict::MSG_SET_HOBBS_TIME:
			handleSetHobbs(m);
			break;

		default:
			break;
	}

	return false;
}

bool cSysMon::handleSetCFG(uKMsg *m)
{
	if(!m)
		return false;

	dbg_printf(2, "Valid CFG message\n");

	float range = 0;
	int port = -1;
	float upper = 600;
	float lower = -600;
	cyg_uint32 logTime = 0;

	uKTag* t = m->firstTag();
	while(t)
	{
		if(t->id() == RMMdict::TAG_PORT_NUM)
		{
			cyg_uint8 p;
			t->data(&p, 1);
			port = p;
		}

		if(t->id() == RMMdict::TAG_DIFF_RANGE)
		{
			t->data(&range, 4);
		}

		if(t->id() == RMMdict::TAG_INPUT_UPPER)
		{
			t->data(&upper, 4);
		}

		if(t->id() == RMMdict::TAG_INPUT_LOWER)
		{
			t->data(&lower, 4);
		}


		if(t->id() == RMMdict::TAG_LOG_INTERVAL)
		{
			t->data(&logTime, 4);
		}

		t = m->nextTag();
	}

	//set port range
	if(port >= 0 && range != 0)
	{
		cNVM::get()->setSampleRange(port, range);
		if(mDebugLevel >= 1)
		{
			printf("Try set DIFF_RANGE to %0.1f on port %d\n", range, port);
		}
	}

	//set port upper limit
	if(port >= 0 && upper < 600)
	{
		cNVM::get()->setUpperLimit(port, upper);
		if(mDebugLevel >= 1)
		{
			printf("Try set INPUT_UPPER to %0.1f on port %d\n", upper, port);
		}
	}

	//set port lower limit
	if(port >= 0 && lower > -600)
	{
		cNVM::get()->setLowerLimit(port, lower);
		if(mDebugLevel >= 1)
		{
			printf("Try set INPUT_LOWER to %0.1f on port %d\n", lower, port);
		}
	}

	//set log interval
	if(logTime)
	{
		cNVM::get()->setLogPeriod(logTime);
		dbg_printf(2, "Try set LOG_INTERVAL to %d\n", logTime);
	}

	return false;
}

bool cSysMon::handleSetTime(uKMsg *m)
{
	if(!m)
		return false;

	diag_printf("Valid set system time Message\n");

	//set the time
	time_t now = 0;
	uKTag* t = m->firstTag();
	while(t)
	{
		if(t->id() == RMMdict::TAG_TIME)
		{
			t->data(&now, 4);
			break;
		}

		t = m->nextTag();
	}

	if(now)
	{
		//diag_printf("Time in message: %s", ctime(&now));
		cRTC::get()->setTime(&now);
		return true;
	}


	return false;
}

bool cSysMon::handleSetHobbs(uKMsg *m)
{
	if(!m)
		return false;

	diag_printf("Valid set Hobbs time Message\n");

	cyg_uint8 port = 0xFF;
	cyg_uint32 onTime = 0xFFFFFFFF;

	uKTag* t = m->firstTag();
	while(t)
	{
		switch(t->id())
		{
			case RMMdict::TAG_PORT_NUM:
				t->data(&port, 1);
				break;

			case RMMdict::TAG_TIME:
				t->data(&onTime, 4);
				break;

			default:
				break;
		}

		t = m->nextTag();
	}

	if(port != 0xFF && onTime != 0xFFFFFFFF)
	{
		cHobbsTimer::get()->setTime(port, onTime);
		return true;
	}


	return false;
}


bool cSysMon::isMyMessage(uKMsg *m)
{
	if(!m)
		return false;

	cyg_uint32 sn = 0;
	cyg_uint64 box = 0;
	uKTag* t = m->firstTag();
	while(t)
	{
		if(t->id() == RMMdict::TAG_RMM_SERIAL)
		{
			t->data(&sn, 4);
			dbg_printf(2, "RMM Serial: 0x%08X\n", sn);
			if(sn != cNVM::get()->getSerial())
				sn = 0;
		}

		if(t->id() == RMMdict::TAG_BOX_SERIAL)
		{

			t->data(&box, 8);
			dbg_printf(2, "Got Box %10llu\n", box);
			if(box != cNVM::get()->getBoxSerial())
				box = 0;
		}

		if(sn && box)
			break;

		t = m->nextTag();
	};

	return (sn && box);
}

bool cSysMon::insertCfg(cyg_uint16 &idx, cyg_uint8* buff,  cyg_uint16 buffLen)
{
	cyg_uint16 len = buffLen;
	cyg_uint32 val;
	cyg_uint8* p;

	uKMsg m(RMMdict::MsgDict, RMMdict::MSG_SET_MDM_CNF);
	val = cNVM::get()->getSerial();
	m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_RMM_SERIAL, &val));
	cyg_uint64 box =  cNVM::get()->getBoxSerial();
	m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_BOX_SERIAL, &box));
//	cyg_uint8 version[3];
//	version[0] = (cNVM::get()->getVersion() & 0xFF0000)>>16;
//	version[1] = (cNVM::get()->getVersion() & 0xFF00)>>8;
//	version[2] = (cNVM::get()->getVersion() & 0xFF);
//	m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_VERSION_STRING, &version));
	val = cNVM::get()->getVersion() ;
	m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_VERSION_STRING, &val));
	p = (cyg_uint8*)cNVM::get()->getSimCell();
	m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_CELL, p));
	p = (cyg_uint8*)cNVM::get()->getSimPin();
	m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_PIN, p));
	p = (cyg_uint8*)cNVM::get()->getSimPuk();
	m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_PUK, p));

	//m.print();
	len = m.getBuff(&buff[idx + 1], len);

	if((idx + len + 2) > buffLen)
		return false;

	buff[idx + len + 1] = cCrc::crc8(&buff[idx + 1], len);
	buff[idx] = len;
	len++;
	idx += len + 1;

	return true;
}

bool cSysMon::insertTime(cyg_uint8 port, cyg_uint16 &idx, cyg_uint8* buff,  cyg_uint16 buffLen)
{
	cyg_uint16 len = buffLen;
	cyg_uint32 val;

	uKMsg m(RMMdict::MsgDict, RMMdict::MSG_SET_HOBBS_TIME);
	val = cNVM::get()->getSerial();
	m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_RMM_SERIAL, &val));
	cyg_uint64 box =  cNVM::get()->getBoxSerial();
	m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_BOX_SERIAL, &box));
	val = port;
	m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_PORT_NUM, &val));
	val = cHobbsTimer::get()->getTime(port);
	m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_TIME, &val));
//	m.print();
	len = m.getBuff(&buff[idx + 1], len);

	if((idx + len + 2) > buffLen)
		return false;

	buff[idx + len + 1] = cCrc::crc8(&buff[idx + 1], len);
	buff[idx] = len;
	len++;
	idx += len + 1;

	return true;
}

bool cSysMon::insertEvent(cEvent* e, cyg_uint16 &idx, cyg_uint8* buff,  cyg_uint16 buffLen)
{
	cyg_uint16 len = buffLen;
	cyg_uint32 val;

	if(!e)
		return false;

	if(e->getType() == cEvent::EVENT_TEMP)
	{
		uKMsg m(RMMdict::MsgDict, RMMdict::MSG_SET_SAMPLE);
		val = cNVM::get()->getSerial();
		m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_RMM_SERIAL, &val));
		cyg_uint64 box = cNVM::get()->getBoxSerial();
		m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_BOX_SERIAL, &box));

		val = e->getSeq();
		m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_SEQUENCE, &val));
		val = e->getPort();
		m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_PORT_NUM, &val));
		val = e->getTimeStamp();
		m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_TIME, &val));
		float temp = e->getTemp();
		unsigned char* p = (unsigned char*)&temp;
		memcpy(&val, p, 4);
		m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_DEGREE, &val));

		//    m.print();
		len = m.getBuff(&buff[idx + 1], len);
	}

	if(e->getType() == cEvent::EVENT_INPUT)
	{
		uKMsg m(RMMdict::MsgDict, RMMdict::MSG_SET_INPUT_STATE);
		val = cNVM::get()->getSerial();
		m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_RMM_SERIAL, &val));
		cyg_uint64 box = cNVM::get()->getBoxSerial();
		m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_BOX_SERIAL, &box));

		val = e->getSeq();
		m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_SEQUENCE, &val));
		val = e->getPort();
		m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_PORT_NUM, &val));
		val = e->getTimeStamp();
		m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_TIME, &val));
		cyg_uint8 state = e->getState();
		m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_INPUT_STATE, &state));

		//    m.print();
		len = m.getBuff(&buff[idx + 1], len);
	}



    if((idx + len + 2) > buffLen)
        return false;

    buff[idx + len + 1] = cCrc::crc8(&buff[idx + 1], len);
    buff[idx] = len;
    len++;
    idx += len + 1;

    return true;
}

bool cSysMon::insertPwrChange(cyg_uint16 &idx, cyg_uint8* buff,  cyg_uint16 buffLen)
{
	cyg_uint16 len = buffLen;
	cyg_uint32 val;
	time_t powerDown = cRTC::get()->getPowerDown();
	time_t powerUp = cRTC::get()->getPowerUp();

	//Power was not lost, it was a system reset
	if(powerUp == 0 || powerDown == 0)
		return true;


	uKMsg m(RMMdict::MsgDict, RMMdict::MSG_POWER_CHANGE);
	val = cNVM::get()->getSerial();
	m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_RMM_SERIAL, &val));
	cyg_uint64 box = cNVM::get()->getBoxSerial();
	m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_BOX_SERIAL, &box));

	val = powerDown;
	m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_POWER_DOWN, &val));
	val = powerUp;
	m.addTag(new uKTag(RMMdict::TagDict, RMMdict::TAG_POWER_UP, &val));

	//    m.print();
	len = m.getBuff(&buff[idx + 1], len);

	if((idx + len + 2) > buffLen)
		return false;

	buff[idx + len + 1] = cCrc::crc8(&buff[idx + 1], len);
	buff[idx] = len;
	len++;
	idx += len + 1;


	diag_printf("SESSION: POWER FAIL ADDED\n");
	return true;
}


bool cSysMon::waitReply(cyg_uint8* buff, cyg_uint16 len)
{
	cyg_uint8 cnt = 0;

	do
	{
		dbg_printf(1, "send msg\n");
		dbg_printf(3, "SYSMON: TX: %d\n", len);
		dbg_dump_buf(3, buff,len);

		mRXlen = 0;
		replied = false;
		cyg_uint16 sent = cModem::get()->send(buff, len);
		dbg_printf(1, "msg sent\n");

		if(!replied)
		{
			cyg_uint32 t = cyg_current_time();
			if(sent > 0 && cyg_cond_timed_wait(&mWaitCond, t + 5000))
			{
				return true;
			}
			else
			{
				diag_printf("SYSMON: Timed out\n");
			}
		}
		else if(mRXlen > 0)
			return true;

	}while(cnt++ < 3);

	return false;
}

void cSysMon::dataReceived(cyg_uint8 *buff, cyg_uint16 len)
{
	while(replied)
			cyg_thread_delay(10);

	if(len > 0)
	{
		cyg_mutex_lock(&mBuffMutex);

		mRXlen = len;
		memcpy(mRXbuff, buff, len);

		cyg_mutex_unlock(&mBuffMutex);

		replied = true;
		cyg_cond_signal(&mWaitCond);
	}
}

/**This determines whether the modem should be switched off
 * after logs have been loaded to the RMM server
 */
void cSysMon::setPowerStat(bool stat)
{
	mPowerStat = stat;
}

void cSysMon::showStat()
{
	dbg_printf(green, "Sysmon status\n");
	diag_printf(" - Last transfer %s", ctime(&mLastSession));
	diag_printf(" - Session Update period: %ds\n", cNVM::get()->getLogPeriod());
	diag_printf(" - Upload fail Count: %d\n", mUploadFailCnt);

	diag_printf(" - Modem pwrStat:\n ");
	if(mPowerStat)
		dbg_printf(green, "\t\tON");
	else
		dbg_printf(red, "\t\tOFF");
}

void cSysMon::printMessage(uKMsg* m)
{
	if(!m)
		return;

	switch(m->id())
	{
		case RMMdict::MSG_SET_TIME:
			diag_printf("Valid SET_TIME msg\n");
			break;
		case RMMdict::MSG_SET_LOG_CNF:
			diag_printf("Valid LOG_CNF msg\n");
			break;
		case RMMdict::MSG_SET_SAMPLE:
			diag_printf("Valid SET msg\n");
			break;

		case RMMdict::MSG_REQ:
			diag_printf("Valid REGUEST msg\n");
			break;

		case RMMdict::MSG_ACK:
			diag_printf("Valid Acknowledge msg\n");
			break;

		default:
			diag_printf("Unknown msg ID: 0x%02X\n", m->id());
			return;
	}

	uKTag *t = m->firstTag();
    while(t)
    {
    	switch(t->id())
    	{
    		case RMMdict::TAG_RMM_SERIAL:
    		{
    			cyg_uint32 sn;
    			t->data(&sn, 4);
    			diag_printf("RMM Serial: 0x%08X\n", sn);
    		}
    			break;

    		case RMMdict::TAG_BOX_SERIAL:
    		{
    			cyg_uint32 sn;
    			t->data(&sn, 4);
    			diag_printf("BOX Serial: 0x%08X\n", sn);
    		}
    			break;

    		case RMMdict::TAG_PORT_NUM:
    		{
    			cyg_uint8 num;
    			t->data(&num, 1);
    			diag_printf("PORT #: %d\n", num);
    		}
    			break;

    		case RMMdict::TAG_TIME:
    		{
    			time_t time;
    			t->data((cyg_uint8*)&time, 4);
    			diag_printf("TIME: %s", ctime(&time));
    		}
    			break;

    		case RMMdict::TAG_SEQUENCE:
    		{
    			cyg_uint32 s;
    			t->data(&s, 4);
    			diag_printf("Sequence: 0x%08X\n", s);
    		}
    			break;

    		case RMMdict::TAG_DEGREE:
    		{
    			float sample;
    			t->data(&sample, 4);
    			printf("Degree: %0.1f\n", sample);
    		}
    			break;

    		case RMMdict::TAG_DIFF_RANGE:
    		{
    			float range;
    			t->data(&range, 4);
    			printf("Range: %0.1f\n", range);
    		}
    			break;

    		case RMMdict::TAG_LOG_INTERVAL:
    		{
    			time_t time;
    			t->data((cyg_uint8*)&time, 4);
    			diag_printf("TIME: %d\n", time);
    		}
    			break;

    		default:
    			break;
    	}

        t = m->nextTag();
    }
    ;
}

time_t cSysMon::getLastSync()
{
	return mLastSession;
}

void cSysMon::setUploadDone(bool stat)
{
	if(stat)
	{
	cLED::get()->indicate(cLED::idle);
	mUploadFailCnt = 0;

	dbg_printf(green, "SESSION: Transfer set as Successful\n");

	mLastSession = cRTC::get()->timeNow();
	}
	else
	{
		cLED::get()->indicate(cLED::connecting);
		mUploadFailCnt = 0;

		dbg_printf(green, "SESSION: Transfer set as pending\n");

		mLastSession = 0;
	}
}

cSysMon::~cSysMon()
{
}

