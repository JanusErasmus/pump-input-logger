#include <stdio.h>
#include <stdlib.h>

#include "sys_mon.h"

#include "MCP_rtc.h"
#include "nvm.h"
#include "crc.h"
#include "input_port.h"
#include "picaxe_lcd.h"
#include "output_port.h"
#include "log.h"

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
	replied = false;
	mPumpStatus = false;
	mPumpStartTime = 0;
	mPumpIdleTime = 0;
	mPumpTimeLeft = 0;
	mPumpInFrameFlag = false;
	mPumpDownTime = false;

	cPICAXEserialLCD::init(SERIAL_CONFIG_DEVICE);
	mMenu = new cStandbyMenu(cPICAXEserialLCD::get());

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

	QAction(new cActionQueue::s_action(cActionQueue::plainAction, sysmonHandlePump));

}

void cSysMon::sys_thread_func(cyg_addrword_t arg)
{
	cSysMon *t = (cSysMon *)arg;
	cyg_bool monStat = false;

	t->mMenu->open();

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
			//switch LCD back light on
			cOutput::get()->setPortState(0,0);

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

	diag_printf("SYSMON: IDLE\n");

	QAction(new cActionQueue::s_action(cActionQueue::plainAction, sysmonHandlePump));

	//switch LCD back light off
	cOutput::get()->setPortState(0,1);
	if(mMenu)
	{
		delete mMenu;
		mMenu = new cStandbyMenu(cPICAXEserialLCD::get(), 0,
				mPumpStatus,
				cInput::get()->getPortState(5),
				mPumpInFrameFlag,
				mPumpTimeLeft);
		mMenu->open();
	}

	return false;
}

cyg_bool cSysMon::handleAction(cyg_addrword_t action)
{
	switch(action)
	{
	case sysmonHandlePump:
	{
		time_t now = cRTC::get()->timeNow();

		//Switch pump on when in time frame and tank level is low
		if(!cInput::get()->getPortState(5))
		{
			struct tm * info = localtime(&now);

			if((cNVM::get()->getPumpFrameStart() <= info->tm_hour) && (info->tm_hour < cNVM::get()->getPumpFrameEnd()))
			{
				mPumpInFrameFlag = true;

					//stop pump if it has been running for set Interval
					if(mPumpStartTime && (now - mPumpStartTime) > (cNVM::get()->getPumpUpTime() * 60))
					{
						//start the pump again after delay for set Interval
						if(mPumpIdleTime && (now - mPumpIdleTime) > (cNVM::get()->getPumpRestTime() * 60))
						{
							mPumpIdleTime = 0;
							mPumpStartTime = now;
							diag_printf("SYSMON: PUMP Restarted\n");
							startPump(now);

							((cStandbyMenu*)mMenu)->setRestingState(0);
						}
						else
						{
							if(!mPumpIdleTime)
								mPumpIdleTime = now;

							diag_printf("SYSMON: PUMP Resting %s", ctime(&mPumpIdleTime));
							stopPump(now);

							((cStandbyMenu*)mMenu)->setRestingState(1);
						}

						mPumpTimeLeft = (cNVM::get()->getPumpRestTime() * 60) - (now - mPumpIdleTime) ;
						diag_printf("left until re-start %d\n", mPumpTimeLeft);
					}
					else
					{
						if(!mPumpStartTime)
							mPumpStartTime = now;

						diag_printf("SYSMON: PUMP Started %s", ctime(&mPumpStartTime));
						mPumpTimeLeft = (cNVM::get()->getPumpUpTime() * 60) - (now - mPumpStartTime) ;
						diag_printf("left until rest %d\n", mPumpTimeLeft);
						startPump(now);
					}

			}
			else
			{
				diag_printf("SYSMON: PUMP force stop\n");
				mPumpIdleTime = 0;
				mPumpStartTime = 0;
				mPumpTimeLeft = 0;
				stopPump(now);
				mPumpInFrameFlag = false;
			}
			((cStandbyMenu*)mMenu)->setInFrameState(mPumpInFrameFlag);
		}
		else //always switch pump off
		{
			mPumpIdleTime = 0;
			mPumpStartTime = 0;
			mPumpTimeLeft = 0;
			diag_printf("SYSMON: PUMP Stopped\n");
			((cStandbyMenu*)mMenu)->setRestingState(0);
			stopPump(now);
		}
		((cStandbyMenu*)mMenu)->setTimeLeft(mPumpTimeLeft);
	}
	break;
	default:
		break;
	}
	return true;
}
void cSysMon::startPump(time_t now)
{
	if(!mPumpStatus)
	{
		mPumpStatus = true;
		cOutput::get()->setPortState(1,1);

		((cStandbyMenu*)mMenu)->setPumpState(1);

		//log pump event
		cEvent e((cyg_uint8)5, (cyg_uint8)1, now);
		e.showEvent();
		cLog::get()->logEvent(&e);
	}
}

void cSysMon::stopPump(time_t now)
{
	if(mPumpStatus)
	{
		mPumpStatus = false;
		cOutput::get()->setPortState(1,0);

		((cStandbyMenu*)mMenu)->setPumpState(0);

		//log pump event
		cEvent e((cyg_uint8)5, (cyg_uint8)0, now);
		e.showEvent();
		cLog::get()->logEvent(&e);

	}
}

cyg_bool cSysMon::handleEvent(s_event* evt)
{
	//digital input
	if(evt->portNumber != 0xFF)
	{
		//only  handle button releases
		if(!evt->state)
		{
			switch(evt->portNumber)
			{
			case 0:
					mMenu->up();
				break;
			case 1:
					mMenu->down();
				break;
			case 2:
					mMenu->enter();
				break;
			case 3:
					mMenu->cancel();
				break;
			}
		}

		//always handle pump input
		if(evt->portNumber == 5)
		{
			diag_printf("SYSMON: Input %d : %s\n", evt->portNumber, evt->state?"close":"open");
			((cStandbyMenu*)mMenu)->setTankLevel(cInput::get()->getPortState(5));

			QAction(new cActionQueue::s_action(cActionQueue::plainAction, sysmonHandlePump));
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
	diag_printf("SYSMON: Handling SMS command %s\n", sms->mText);
	//diag_dump_buf((void*)command, strlen(command));


}

bool cSysMon::placeMissedCall()
{
	bool status = false;

	diag_printf("SYSMON: placing missed call\n");

	if(cModem::get()->missedCall(cNVM::get()->getSimCell()))
	{
		status = true;
	}
	else
	{
		diag_printf("SYSMON: missed call failed\n");
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

void cSysMon::navigate(cTerm & term, int argc,char * argv[])
{
	if(!_instance)
		return;

	if(argc > 1)
	{
		if(!strcmp(argv[1], "e"))
		{
			_instance->mMenu->enter();
		}
		if(!strcmp(argv[1], "c"))
		{
			_instance->mMenu->cancel();
		}
		if(!strcmp(argv[1], "u"))
		{
			_instance->mMenu->up();
		}
		else if(!strcmp(argv[1], "d"))
		{
			_instance->mMenu->down();
		}
	}
	else
	{
		diag_printf("Specify direction\n");
	}
}

cSysMon::~cSysMon()
{
}

