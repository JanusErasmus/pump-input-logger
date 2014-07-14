#include <stdio.h>
#include <stdlib.h>

#include "sys_mon.h"

#include "MCP_rtc.h"
#include "nvm.h"
#include "crc.h"
#include "input_port.h"

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
	mAlarmAck = true;
	mAlarmEnabled = false;
	mAlarmTimer = 0;
	mAlarmDisEnable = false;
	mWalkOutTime = 0;
	mAlarmFrameState = false;
	mTestMissedCallFlag = false;

	mRXlen = 0;
	replied = false;
	mUploadFailCnt = 0;
	mPowerFlag = false;

	mWatchDog = new wdKicker(300);

	cyg_mutex_init(&mMonitorMutex);


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
	cyg_bool monStat = false;

	for(;;)
	{

		t->mWatchDog->reset();
		cyg_mutex_lock(&t->mMonitorMutex);
		monStat = t->monitor();
		cyg_mutex_unlock(&t->mMonitorMutex);
	}
}

cyg_bool cSysMon::monitor()
{
	s_action  * act = (s_action *)cyg_mbox_timed_get(mActionQHandle, cyg_current_time() + 15000);
	if (act)
	{
		cyg_bool stat = false;
		switch(act->type)
		{
		case event:
		{
			s_event * evt = (s_event*)act->action;
			if(evt)
			{
				stat = handleEvent(evt);
				delete evt;
			}
		}
		break;

		case plainAction:
			stat = handleAction(act->action);
			break;

		default:
			break;
		}

		delete act;

		return stat;
	}



	return false;
}

void cSysMon::updateAlarmState()
{

}

void cSysMon::armDisarm()
{
	if(!mWalkOutTime)
	{
		if(mAlarmEnabled)
		{
			disarm();
		}
		else
		{
			arm();
		}

		mWalkOutTime = 3;
	}

	mWalkOutTime--;
	diag_printf("SYSMON: Walkout %d\n", mWalkOutTime);

	if(mWalkOutTime == 0)
	{
		mAlarmDisEnable = false;
	}
}

void cSysMon::arm()
{
	if(mAlarmEnabled)
		return;

	mAlarmEnabled = true;
	cLED::get()->disable();
	cLED::get()->indicate(cLED::walkOut);

	diag_printf("SYSMON: ARMED\n");
}

void cSysMon::disarm()
{
	if(!mAlarmEnabled)
		return;

	mAlarmEnabled = false;
	cLED::get()->enable();
	diag_printf("SYSMON: DISARMED\n");
}

cyg_bool cSysMon::handleAction(cyg_addrword_t action)
{
	if(!(cModem::get()->getModemStatus() == cModem::GPRSatt || cModem::get()->getModemStatus() == cModem::NetworkRegistered))
	{
		diag_printf("Wait for modem to register\n");
		QAction(new s_action(plainAction, action));

		return false;
	}

	switch(action)
	{
	case sysmonActionTest:
		{
			time_t now = cRTC::get()->timeNow();
			diag_printf("SYSMON: Try placing Test missed call %s", asctime(localtime(&now)));

			if(placeMissedCall())
			{
				now = cRTC::get()->timeNow();

				diag_printf("SYSMON: Test missed call placed  %s", asctime(localtime(&now)));
			}
			else
			{
				QAction(new s_action(plainAction, sysmonActionTest));
			}
		}
		break;

	case sysmonActionAlarm:
	{
		static cyg_uint32 missedCallHeader = 0;
		time_t now = cRTC::get()->timeNow();

		//when alarm was enabledDisbaled ignore alarms
		if(mAlarmDisEnable)
			break;

		//only handle alarm action every 5 minutes
		if(now < mAlarmTimer + 300)
		{
			diag_printf("SYSMON: Alarm was handled less than 5 min. ago\n");
			break;
		}
		mAlarmTimer = now;

		if(mAlarmEnabled && !mAlarmAck)
		{
			if(!missedCallHeader)
			{
				diag_printf("SYSMON: Alarm! try placing missed call %s", asctime(localtime(&now)));
				missedCallHeader = now;
			}

			if(placeMissedCall())
			{
				now = cRTC::get()->timeNow();

				diag_printf("SYSMON: missed call placed (%ds) - %s", now - missedCallHeader, asctime(localtime(&now)));
				missedCallHeader = 0;

//				cModem::get()->power(0);
			}
			else
			{
				mAlarmTimer = 0;
				QAction(new s_action(plainAction, sysmonActionAlarm));
			}
		}
		else
		{
			diag_printf("SYSMON: Alarm disabled NOT placing missed call\n");
			mAlarmAck = true;
		}
	}
	break;

	case sysmonActionPowerLoss:
		if(sendPowerSMS(0))
		{
			mPowerFlag = false;
			diag_printf("SYSMON: SMS sent\n");
		}
		else
		{
			QAction(new s_action(plainAction, sysmonActionPowerLoss));
		}
		break;
	case sysmonActionPowerRestored:
		if(sendPowerSMS(1))
		{
			mPowerFlag = false;
			diag_printf("SYSMON: SMS sent\n");
		}
		else
		{
			QAction(new s_action(plainAction, sysmonActionPowerRestored));
		}
		break;

	case sysmonActionSMS:
		handleSMSlist();
		break;
	}

	return true;
}

cyg_bool cSysMon::handleEvent(s_event* evt)
{

	//digital input
	if(evt->portNumber != 0xFF)
	{
		diag_printf("SYSMON: Input %d : %s\n", evt->portNumber, evt->state?"close":"open");

		if(evt->portNumber == POWER_IN_PORT)
		{
			diag_printf("SYSMON: Power Changed\n");
			//only queue one alarm action until it has been handled
			if(!mPowerFlag)
			{
				mPowerFlag = true;
				if(evt->state)
					QAction(new s_action(plainAction, sysmonActionPowerLoss));
				else
					QAction(new s_action(plainAction, sysmonActionPowerRestored));
			}
			return true;
		}

		if(evt->portNumber == ALARM_SET_PORT)
		{
			if(!mAlarmDisEnable)
			{
				diag_printf("SYSMON: ALARM SET/RESET\n");
				mAlarmDisEnable = true;
				return false;
			}
		}
		else
		{
			time_t now = cRTC::get()->timeNow();
			struct tm* info = localtime(&now);
			diag_printf("SYSMON: [%d:%d]\n", info->tm_wday, info->tm_hour);

			if(!mAlarmTimer)
			{
				diag_printf("SYSMON: Setting Alarm\n");
				mAlarmAck = false;
				QAction(new s_action(plainAction, sysmonActionAlarm));
			}
			else
			{
				diag_printf("SYSMON: Alarm was handled less than 5 min. ago\n");
			}
		}
	}

	return true;
}

void cSysMon::handleSMSlist()
{

	cMdmReadSMS::sSMS * list[10];
	for(cyg_uint8 k = 0; k < 10; k++)
		list[k] = 0;

	if(cModem::get()->getSMSlist(list))
	{
		diag_printf("SYSMON: Handling SMS's\n");

		cyg_uint8 k = 0;
		while(list[k])
		{
			//Only handle SMS from members in the phone book (phone book entries has names)
			if(list[k]->mName[0])
			{
				diag_printf("SYSMON: SMS from %s\n", list[k]->mName);
				handleSMScommand(list[k]);
			}
			else
			{
				diag_printf("SYSMON: SMS\n %s\n", list[k]->mText);
			}

			//list[k]->show();
			delete list[k];
			k++;
		}

		diag_printf("SYSMON: Delete All read SMS\n");
		cModem::get()->deleteSMS();
	}

	diag_printf("SYSMON: Handling SMS done\n");
}

void cSysMon::handleSMScommand(cMdmReadSMS::sSMS * sms)
{
	diag_printf("SYSMON: Handling command %s\n", sms->mText);
	//diag_dump_buf((void*)command, strlen(command));
	if(!strcmp(sms->mText,"Disable\r"))
	{
		diag_printf("SYSMON: Disable alarm\n");
		mAlarmEnabled = false;
	}
	else if(!strcmp(sms->mText,"Enable\r"))
	{
		diag_printf("SYSMON: Enable alarm\n");
		mAlarmEnabled = true;
	}
	else if(!strcmp(sms->mText,"Status\r"))
	{
		char txtMsg[160];
		diag_printf("SYSMON: Reply Status\n");

		sprintf(txtMsg, "%s\n\nPIR: %s\nMiddle Door: %s\nReception Door: %s\nEmployee Door: %s\n", mAlarmEnabled?"ARMED":"DISARMED"
						, cInput::get()->getPortState(0)?"HIGH":"LOW"
						, cInput::get()->getPortState(1)?"CLOSED":"OPEN"
						, cInput::get()->getPortState(2)?"OPEN":"CLOSED"
						, cInput::get()->getPortState(3)?"OPEN":"CLOSED"
				);

		diag_printf("SYSMON: SMS %s\n%s\n", sms->mNumber, txtMsg);
		cModem::get()->sendSMS(sms->mNumber, txtMsg);
	}

}

bool cSysMon::placeMissedCall()
{
	bool status = false;
	bool stat = 0;

	diag_printf("SYSMON: placing missed call\n");

	if(cNVM::get()->getCallIndex())
		stat = cModem::get()->missedCall(cNVM::get()->getCallIndex());
	else
		stat = cModem::get()->missedCall(cNVM::get()->getSimCell());


	if(stat)
	{
		status = true;
	}
	else
	{
		diag_printf("SYSMON: missed call failed\n");

		if(mUploadFailCnt++ > 2)
		{
			mUploadFailCnt = 0;

			cLED::get()->indicate(cLED::waitNetwork);

			diag_printf("SYSMON: Reset modem\n");
			cModem::get()->power(0);
			cyg_thread_delay(10000);

		}
	}

	return status;
}

bool cSysMon::sendPowerSMS(cyg_bool state)
{
	bool TXstat = true;
	char lossMsg[] = "Power Loss";
	char restoreMsg[] = "Power Restore";
	char* txtMsg;

	if(state)
	{
		txtMsg = restoreMsg;
	}
	else
	{
		txtMsg = lossMsg;
	}

	//SMS all persons in the phonebook
	cMdmGetPB::s_entry list[20];
	int pbSize = 20;
	int pbCount = 0;

	//try read phone book 3 times each 5s
	int retryCnt = 2;
	bool TXdone = false;
	do
	{
		if(cModem::get()->getPhoneBook(list, &pbCount, &pbSize))
		{
			for(int k = 0; k < pbCount; k++)
			{
				diag_printf("SYSMON: SMS %s to %s\n", txtMsg, list[k].number);
				TXstat = cModem::get()->sendSMS(list[k].number, txtMsg);
				if(!TXstat)
					break;
				retryCnt = 2;
			}

			TXdone = true; //end do while
		}
		else
		{
			cyg_thread_delay(2500); //wait 5s
		}
	}while(!TXdone && retryCnt--);

	return TXstat;
}


void cSysMon::acknowledgeAlarm()
{
	mAlarmAck = true;
}

cLED::eLEDstatus cSysMon::registerModem()
{
	static cyg_bool setDefaults = false;

	cModem::get()->updateStatus();
	cModem::eModemStat stat = cModem::get()->getModemStatus();

	//when modem is off, power it up
	if(stat == cModem::Off)
	{
		diag_printf("SYSMON: Powering modem\n");
		cModem::get()->power(1);

		//wait for modem to initialize
		cyg_thread_delay(2000);

		stat = cModem::get()->getModemStatus();
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

	if(!setDefaults)
		setDefaults =  cModem::get()->setEcho(0); //cModem::get()->setFixedBaud() &&

	switch(stat)
	{
	case cModem::SIMnotReady:
	{
		cLED::eLEDstatus simStat = checkSIM();
		if(simStat)
			return simStat;
	}
	break;

	case cModem::NetworkSearching:
		diag_printf("SYSMON: Searching for Network\n");
		return cLED::waitNetwork;

	case cModem::NetworkBusy:
		diag_printf("SYSMON: Waiting for Network\n");
		return cLED::waitNetwork;

	case cModem::GPRSatt:
	case cModem::NetworkRegistered:
		diag_printf("SYSMON: Registered\n");
		return (cLED::eLEDstatus)0;

	case cModem::Off:
		diag_printf("SYSMON: Modem is OFF\n");
		return cLED::couldNotPwrModem;

	default:
		diag_printf("SYSMON: Modem stat %d\n", stat);
		return cLED::waitNetwork;

	}

	return (cLED::eLEDstatus)0;
}

cLED::eLEDstatus cSysMon::checkSIM()
{

	cModem::eSIMstat status = cModem::get()->getSIMstatus();

	switch(status)
	{
	case cModem::SIMneeded:
		diag_printf("SYSMON: Insert SIM - Powering down modem\n");
		cModem::get()->power(0);
		return cLED::SIMerr;

	case cModem::SIMpuk:
	{
		if(!cNVM::get()->getSimPukFlag() && cModem::get()->insertPUK(cNVM::get()->getSimPuk(), cNVM::get()->getSimPin()))
		{
			//wait for PIN accepted
			cyg_thread_delay(500);
			cModem::get()->updateStatus();
		}
		else
		{
			cNVM::get()->setSimPukFlag(true);
			diag_printf("SYSMON: SIM blocked, PUK needed\n");
			return cLED::PUKerr;
		}
	}
	break;

	case cModem::SIMpin:
		if(cModem::get()->insertPIN(cNVM::get()->getSimPin()))
		{
			//wait for PIN accepted
			cNVM::get()->setSimPukFlag(false);
			cyg_thread_delay(500);

			cModem::get()->updateStatus();
		}
		else
		{
			diag_printf("SYSMON: incorrect PIN\n");
			return cLED::PINerr;
		}
		break;

	case cModem::SIMready:
		break;
	}

	return (cLED::eLEDstatus)0;

}

void cSysMon::setPowerStat(cTerm & term, int argc,char * argv[])
{
	if(!_instance)
		return;

	if(argc > 1)
	{
		bool flag = strtoul(argv[1],NULL,10);
		term<<"pressing modem pwrKey button - ";

		if(flag)
		{
			term<<"on\n";
		}
		else
		{
			term<<"off\n";
		}

		cModem::get()->power(flag);
	}
}



cSysMon::~cSysMon()
{
}
