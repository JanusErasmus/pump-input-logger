#include <cyg/kernel/diag.h>
#include <cyg/io/ttyio.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "modem.h"
#include "utils.h"
#include "sys_mon.h"

cModem* cModem::_instance = 0;

void cModem::init(char* serDev)
{
	if(!_instance)
	{
		_instance = new cModem(serDev);
	}
}

cModem* cModem::get()
{
	return _instance;
}

cModem::cModem(char* serDev)
{
	mRXbuff[0] = 0;
	mRXlen = 0;
	mModemStatus = Off;
	mConnection = IPunknown;
	mSIMstatus = SIMneeded;


	mCurrCMD = 0;
	cyg_mutex_init(&mCurrCMDMutex);

	cyg_mutex_init(&mCallBusyMutex);
	cyg_cond_init(&mCallBusyCond, &mCallBusyMutex);

	cyg_mutex_init(&mModemStatusMutex);

	// Modem CMD UART
	Cyg_ErrNo err =	cyg_io_lookup(serDev,&mSerCMDHandle);

	diag_printf("cModem CMD %p: %s \n", mSerCMDHandle, strerror(-err));

	cyg_serial_info_t info;

	info.flags = 0;
	info.baud = CYGNUM_SERIAL_BAUD_19200;
	info.stop = CYGNUM_SERIAL_STOP_1;
	info.parity = CYGNUM_SERIAL_PARITY_NONE;
	info.word_length = CYGNUM_SERIAL_WORD_LENGTH_8;

	cyg_uint32 info_len = sizeof(info);
	cyg_io_set_config(mSerCMDHandle,
			CYG_IO_SET_CONFIG_SERIAL_INFO,
			&info,
			&info_len);

	cyg_tty_info_t tty_info;
	cyg_uint32 len = sizeof(tty_info);
	cyg_io_get_config(mSerCMDHandle,
			CYG_IO_GET_CONFIG_TTY_INFO,
			&tty_info,
			&len);

	//diag_printf("Modem: TTY in_flags = 0x%08X, out_flags = 0x%08X\n",tty_info.tty_in_flags,tty_info.tty_out_flags);

	tty_info.tty_in_flags = 0;

	cyg_io_set_config(mSerCMDHandle,
			CYG_IO_SET_CONFIG_TTY_INFO,
			&tty_info,
			&len);

	// Modem DATA UART
	err =	cyg_io_lookup("/dev/ser1",&mSerDataHandle);
	diag_printf("cModem RX  %p: %s \n", mSerDataHandle, strerror(-err));

	info_len = sizeof(info);
	cyg_io_set_config(mSerDataHandle,
			CYG_IO_SET_CONFIG_SERIAL_INFO,
			&info,
			&info_len);

	cyg_mutex_init(&mRXmutex);

	cyg_thread_create(MDM_PRIOR,
			cModem::rx_thread_func,
			(cyg_addrword_t)this,
			(char *)"Modem",
			mStack,
			MDM_STACK_SIZE,
			&mThreadHandle,
			&mThread);

	mBalance = 0;
	cyg_mutex_init(&mBalanceMutex);
	cyg_cond_init(&mBalanceCond, &mBalanceMutex);

//	cyg_uint32 reg32;
//	HAL_READ_UINT32(CYGHWR_HAL_STM32_UART2 + CYGHWR_HAL_STM32_UART_BRR, reg32);
//	diag_printf("\t BRR: 0x%08X\n", reg32);
//	reg32 = 0x24;
//	HAL_WRITE_UINT32(CYGHWR_HAL_STM32_UART2 + CYGHWR_HAL_STM32_UART_BRR, reg32);

	cyg_thread_resume(mThreadHandle);

}

cModem::eModemStat cModem::getModemStatus()
{
	return mModemStatus;
}

cModem::eSIMstat cModem::getSIMstatus()
{
	return mSIMstatus;
}

void cModem::updateStatus()
{
	cyg_mutex_lock(&mModemStatusMutex);
	mModemStatus = retrieveModemStatus();
	cyg_mutex_unlock(&mModemStatusMutex);
}

cModem::eModemStat cModem::retrieveModemStatus()
{
	if(!showID())
	{
		mSIMstatus = SIMneeded;
		return Off;
	}

	if(mSIMstatus != SIMready)
	{
		mSIMstatus = simStatus();
		if(mSIMstatus != SIMready)
			return SIMnotReady;
	}


	if(isCallReady())
	{
		if(isRegistered())
		{
			if(isGPRSattatched())
				return GPRSatt;

			return NetworkRegistered;
		}

		return NetworkCallReady;
	}

	return NetworkBusy;
}

cModem::eSIMstat cModem::simStatus()
{
	if(!isSIMinserted())
		return SIMneeded;

	cMdmPINstat::ePINstat pinStatus = getPINstat();
	switch(pinStatus)
	{
		case cMdmPINstat::SIM_PIN:
		case cMdmPINstat::SIM_PIN2:
		case cMdmPINstat::PH_SIM_PIN:
			return SIMpin;
		case cMdmPINstat::SIM_PUK:
		case cMdmPINstat::SIM_PUK2:
		case cMdmPINstat::PH_SIM_PUK:
			return SIMpuk;
		case cMdmPINstat::READY:
			return SIMready;
		default:
			return SIMneeded;
	}
}

void cModem::showSIMStatus(eSIMstat stat)
{
	switch(stat)
	{
	case SIMneeded:
		diag_printf("SIM: Insert SIM\n");
		break;
	case SIMpin:
		diag_printf("SIM: insert PIN\n");
		break;
	case SIMpuk:
		diag_printf("SIM: insert PUK\n");
		break;
	case SIMready:
		diag_printf("SIM: READY\n");
		break;
	}
}

void cModem::showModemStatus(eModemStat stat)
{
	switch(stat)
	{
		case Off:
			diag_printf("Modem: OFF\n");
			break;
		case SIMnotReady:
			diag_printf("Modem: SIM not ready\n");
			break;
		case NetworkBusy:
			diag_printf("Modem: Network Busy\n");
			break;
		case NetworkSearching:
			diag_printf("Modem: Searching Network\n");
			break;
		case NetworkCallReady:
			diag_printf("Modem: Call Ready\n");
			break;
		case NetworkRegistered:
			diag_printf("Modem: Network registered\n");
			break;
		case GPRSatt:
			diag_printf("Modem: GPRS attatched\n");
			break;
		case Linked:
			diag_printf("Modem: LINKED\n");
			break;
	}

}

void cModem::rx_thread_func(cyg_addrword_t arg)
{
	cModem *t = (cModem *)arg;

	for(;;)
	{
		t->run();
	}
}

void cModem::run()
{
//	mRXlen = 1;
//	Cyg_ErrNo err = cyg_io_read(mSerDataHandle, mRXbuff, &mRXlen);
//	if(err < 0)
//		diag_printf("cModemErr: %s \n", strerror(-err));
//
//	diag_dump_buf(mRXbuff, mRXlen);


	mRXlen = MODEM_BUFF_SIZE;
	Cyg_ErrNo err = cyg_io_read(mSerCMDHandle, mRXbuff, &mRXlen);
	if(err < 0)
		diag_printf("cModemErr: %s \n", strerror(-err));

	//discard \n at end of received buffer
	if(mRXlen > 1 && mRXlen < 256)
	{
		if(mRXbuff[mRXlen] != 0)
			mRXbuff[mRXlen] = 0;

		//diag_printf("MODEM: RX %s\n", mRXbuff);

		if(mCurrCMD)
		{
			mCurrCMD->handleData(mRXbuff, mRXlen);
		}
		else
		{
			//diag_printf("URC: %s", mRXbuff);
			handleURC((const char*) mRXbuff);
		}
	}
}

void cModem::handleURC(const char* response)
{
	int cmdLen;

	//When a call is hung up reset alarm
	cmdLen = 4;
	if(!strncmp(response,"BUSY", cmdLen))
	{
		diag_printf("MODEM: Call busy \n");

		cyg_cond_signal(&mCallBusyCond);
		return;
	}


	//Received an SMS
	cmdLen = 6;
	if(!strncmp(response,"+CMTI:", cmdLen))
	{
		diag_printf("MODEM: Received SMS\n");
	//XXX	cSysMon::get()->QAction(new cSysMon::s_action(cSysMon::plainAction, cSysMon::sysmonActionSMS));

		return;
	}

	//USSD URC response
	cmdLen = 6;
	if(!strncmp(response,"+CUSD:", cmdLen))
	{
		char* val = 0;
		//diag_printf("USSD: %s \n", &buff[cmdLen]);

		do
		{
			//diag_printf(" - %c\n", buff[cmdLen]);
			if(response[cmdLen] == 'R')
			{
				val = (char*)&response[cmdLen] + 1;
				break;
			}
		}
		while(response[cmdLen++]);

		if(val)
		{
			//diag_printf("%s\n", val);
			mBalance = atof(val);
			cyg_cond_signal(&mBalanceCond);
		}
		return;
	}

	//Network registration URC
	cmdLen = 6;
	if(!strncmp(response,"+CREG:", cmdLen))
	{
		cyg_uint8 reg = strtoul(&response[cmdLen],0,10);
		cyg_mutex_lock(&mModemStatusMutex);
		switch(reg)
		{
			case 0:
				dbg_printf(1,"CREG: NOT REGISTERED\n");
				mModemStatus = NetworkBusy;
				break;
			case 1:
				dbg_printf(1,"CREG: REGISTERED\n");
				mModemStatus = NetworkRegistered;
				break;
			case 2:
				diag_printf("CREG: SEARCHING\n");
				mModemStatus = NetworkBusy;
				break;
			case 3:
				diag_printf("CREG: DENIED\n");
				break;
			case 4:
				diag_printf("CREG: UNKNOWN\n");
				break;
			case 5:
				diag_printf("CREG: ROAMIMG\n");
				mModemStatus = NetworkRegistered;
				break;
			default:
				break;
		}
		cyg_mutex_unlock(&mModemStatusMutex);
		return;
	}

	//SIM URC
	cmdLen = 7;
	if(!strncmp(response,"+CPIN: ",cmdLen))
	{
		cyg_mutex_lock(&mModemStatusMutex);
		//printf("PIN stat: %s\n", &response[cmdLen]);
		if(!strcmp(&response[cmdLen], "READY\r"))
		{
			mSIMstatus = SIMready;
		}
		else if(!strcmp(&response[cmdLen], "SIM PIN\r"))
		{
			mSIMstatus = SIMpin;
			mModemStatus = SIMnotReady;
		}
		else if(!strcmp(&response[cmdLen], "SIM PUK\r"))
		{
			mSIMstatus = SIMpuk;
			mModemStatus = SIMnotReady;
		}
		else if(!strcmp(&response[cmdLen], "PH_SIM PIN\r"))
		{
			mSIMstatus = SIMpin;
			mModemStatus = SIMnotReady;
		}
		else if(!strcmp(&response[cmdLen], "PH_SIM PUK\r"))
		{
			mSIMstatus = SIMpuk;
			mModemStatus = SIMnotReady;
		}
		cyg_mutex_unlock(&mModemStatusMutex);
	}

	if(!strcmp(response, "Call Ready\r"))
	{
		cyg_mutex_lock(&mModemStatusMutex);
		if(mModemStatus == NetworkBusy)
		{
			mModemStatus = NetworkCallReady;
			dbg_printf(1,"CREG: Call Ready\n");
		}
		cyg_mutex_unlock(&mModemStatusMutex);
		return;
	}

	//IP connection URC responses
	cmdLen = 7;
	if(!strncmp(response,"STATE: ",cmdLen))
	{
		//printf("STAT response: %s\n", &response[cmdLen]);

		if(!strcmp(&response[cmdLen], "IP STATUS\r"))
		{
			mConnection = IPunknown;
			dbg_printf(1, "IPCONN: STATUS\n");
		}
		else if(!strcmp(&response[cmdLen], "IP INITIAL\r"))
		{
			mConnection = IPinitial;
			dbg_printf(1,"IPCONN: IP INITIAL\n");
		}
		else if(!strcmp(&response[cmdLen], "UDP CONNECTING\r"))
		{
			mConnection = IPconnecting;
			dbg_printf(1,"IPCONN: CONNECTING\n");
		}
		else if(!strcmp(&response[cmdLen], "CONNECT OK\r"))
		{
			mConnection = IPconnected;
			dbg_printf(1,"IPCONN: CONNECTED\n");
		}
		else if(!strcmp(&response[cmdLen], "IP GPRSACT\r"))
		{
			mConnection = IPgprsActive;
			dbg_printf(1,"IPCONN: GPRS ACTIVE\n");
		}
		else if(!strcmp(&response[cmdLen], "CONNECTED\r"))
		{
			mConnection = IPconnected;
			dbg_printf(1,"IPCONN: CONNECTED\n");
		}
		else if(!strcmp(&response[cmdLen], "UDP CLOSED\r"))
		{
			mConnection = IPclosed;
			dbg_printf(1,"IPCONN: CLOSED\n");
		}
		return;
	}

	if(!strcmp(response, "CONNECT OK\r"))
	{
		mConnection = IPconnected;
		dbg_printf(1,"IPCONN: CONNECTED\n");
		return;
	}

	if(!strcmp(response, "ALREADY CONNECT\r"))
	{
		mConnection = IPconnected;
		dbg_printf(1,"IPCONN: CONNECTEDd\n");
		return;
	}

	if(!strcmp(response, "SHUT OK\r"))
	{
		mConnection = IPclosed;
		dbg_printf(1,"IPCONN: CLOSED\n");
		return;
	}

	if(!strcmp(response, "CLOSE OK\r"))
	{
		mConnection = IPclosed;
		dbg_printf(1,"IPCONN: CLOSED\n");
		return;
	}

	cmdLen = 4;
	if(!strncmp(response,"RECV", cmdLen) && mConnection == IPconnected)
	{
		receive();
		return;
	}

	dbg_printf(1, "URC: %s\n", response);
}

bool cModem::getSMSlist(cMdmReadSMS::sSMS ** list)
{
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmReadSMS(list);
	stat = mCurrCMD->execute();
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

bool cModem::deleteSMS()
{
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmDeleteSMS();
	stat = mCurrCMD->execute();
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

void cModem::power(bool stat)
{
	if(stat)
	{
		set_pinH(MDM_PWR_OFF);

		//check if modem is not already switched on
		if(showID())
		{
			updateStatus();
			return;
		}
	}
	else
	{
		mModemStatus = Off;
		mSIMstatus = SIMneeded;
	}

	set_pinH(SIMCOM_PWR_KEY);
	cyg_thread_delay(1000);
	set_pinL(SIMCOM_PWR_KEY);

}

void cModem::reset()
{
	diag_printf("Modem: Hard reset toggle on TP1\n");
	set_pinL(MDM_PWR_OFF);
	cyg_thread_delay(1200);
	set_pinH(MDM_PWR_OFF);
}

bool cModem::showID()
{
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmGetID();
	if(mCurrCMD->execute())
	{
		dbg_printf(1,"Modem ID: %s\n", ((cMdmGetID*)(mCurrCMD))->getID());
		stat = true;
	}
	else
	{

	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}



bool cModem::setEcho(bool stat)
{
	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmSetEcho(stat);
	stat = false;

	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

bool cModem::isSIMinserted()
{
	bool simStat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmSIMinserted();
	if(mCurrCMD->execute())
	{
		if(((cMdmSIMinserted*)(mCurrCMD))->inserted()){
			simStat = true;
		}
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return simStat;
}

cMdmPINstat::ePINstat cModem::getPINstat()
{
	cMdmPINstat::ePINstat stat = cMdmPINstat::UNKNOWN;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmPINstat();
	if(mCurrCMD->execute())
	{
		stat = ((cMdmPINstat*)(mCurrCMD))->stat();
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

bool cModem::missedCall(const char* number)
{
	diag_printf("MODEM: calling %s\n", number);
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmPlaceCall(number);
	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;

	if(stat)
	{
		stat = false;

		diag_printf("MODEM: Waiting...\n");
		if(cyg_cond_timed_wait(&mCallBusyCond, cyg_current_time() + 10000))
		{
			diag_printf("MODEM: caller busy\n");
			stat = true;
		}
		else
		{
			diag_printf("MODEM: hang up\n");
			mCurrCMD = new cMdmEndCall();
			if(mCurrCMD->execute())
			{
				stat = true;
			}
			delete mCurrCMD;
			mCurrCMD = 0;
		}
	}
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

bool cModem::missedCall(int index)
{
	diag_printf("MODEM: calling Idx%d\n", index);
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmPlaceCall(index);
	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;

	if(stat)
	{
		stat = false;

		diag_printf("MODEM: Waiting...\n");
		if(cyg_cond_timed_wait(&mCallBusyCond, cyg_current_time() + 10000))
		{
			diag_printf("MODEM: caller busy\n");
			stat = true;
		}
		else
		{
			diag_printf("MODEM: hang up\n");
			mCurrCMD = new cMdmEndCall();
			if(mCurrCMD->execute())
			{
				stat = true;
			}
			delete mCurrCMD;
			mCurrCMD = 0;
		}
	}
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}



bool cModem::sendSMS(const char* number, const char* text)
{
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmSendSMS();
	if(((cMdmSendSMS*)mCurrCMD)->send(number, text))
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

void cModem::checkBalance()
{
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmUSSD("*188#");
	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;

	if(stat)
	{
		if(cyg_cond_timed_wait(&mBalanceCond, cyg_current_time() + 5000))
		{
			printf("Balance is R%0.2f\n", mBalance);
		}
	}

	mCurrCMD = new cMdmUSSDoff();
	mCurrCMD->execute();
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);
}

bool cModem::insertPIN(char* pin)
{
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmEnterPIN(pin);
	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

bool cModem::insertPUK(char* puk, char* pin)
{
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmEnterPIN(puk, pin);
	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}


int cModem::getSQuality()
{
	int signalQ = -999;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmSignalQ();
	if(mCurrCMD->execute())
	{
		signalQ = ((cMdmSignalQ*)(mCurrCMD))->strength();
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return signalQ;
}

bool cModem::isCallReady()
{
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmCallReady();
	if(mCurrCMD->execute())
	{
		stat = ((cMdmCallReady*)(mCurrCMD))->stat();
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

bool cModem::isRegistered()
{
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmNetRegistration();
	if(mCurrCMD->execute())
	{
		stat = ((cMdmNetRegistration*)(mCurrCMD))->stat();
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

bool cModem::getNetOperator(char* netOperator)
{
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmNetOperator();
	if(mCurrCMD->execute())
	{
		strcpy(netOperator, ((cMdmNetOperator*)(mCurrCMD))->name());
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

bool cModem::isGPRSattatched()
{
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmGPRSattched();
	if(mCurrCMD->execute())
	{
		stat = ((cMdmGPRSattched*)((mCurrCMD)))->stat();
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

bool cModem::setGSMalphabet()
{
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmSetGSMalphabet();
	if(mCurrCMD->execute()){
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

bool cModem::setIPstate(cMdmSetIPstate::IPconn conStat)
{
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmSetIPstate();
	//set state
	((cMdmSetIPstate*)(mCurrCMD))->state(conStat);

	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

bool cModem::setIPmode(cMdmSetIPmode::IPmode mode)
{
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmSetIPmode();
	//set state
	((cMdmSetIPmode*)((mCurrCMD)))->mode(mode);
	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

bool cModem::setGPRSconnection(char *apn, char* user_name, char* password)
{
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmSetGPRSconnection(apn, user_name, password);
	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

bool cModem::setPort(cMdmSetLocalPort::Lmode mode, cyg_uint16 port)
{
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmSetLocalPort();

	((cMdmSetLocalPort*)(mCurrCMD))->setPort(mode, port);

	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

bool cModem::setRXIPshow(bool shown)
{
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmSetRXIPshow(shown);
	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

bool cModem::setTransparentCFG(int retryN, int waitN, int buffLen, bool escSeq)
{
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmCfgTransparentTx(retryN, waitN, buffLen, escSeq);
	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

bool cModem::upConnection()
{
	bool stat = false;

	while(!stat){
		//try start task
		cyg_mutex_lock(&mCurrCMDMutex);
		mCurrCMD = new cMdmStartTask();
		stat = mCurrCMD->execute();
		delete mCurrCMD;
		mCurrCMD = 0;
		cyg_mutex_unlock(&mCurrCMDMutex);
		//Couldn't start task, shut down all IP connections
		if(!stat)
		{
			if(shutIP())
				mConnection = IPclosed;
		}
	}

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmUpConnection();
	stat = mCurrCMD->execute();
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

bool cModem::getLocalIP(char *ip)
{
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmGetLocalIP();
	if(mCurrCMD->execute())
	{
		((cMdmGetLocalIP*)(mCurrCMD))->setIP(ip);
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

bool cModem::getDNS(char *priDNS, char *secDNS)
{
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmCfgDNS();
	if(mCurrCMD->execute())
	{
		((cMdmCfgDNS*)((mCurrCMD)))->getDNS(priDNS, secDNS);
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

bool cModem::disablePrompt()
{
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmSetPrompt(cMdmSetPrompt::noPrompt);
	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	if(!stat)
		return false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmSetRXHeader(cMdmSetRXHeader::header);
	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

bool cModem::IPstatus()
{
	bool stat;

	cyg_mutex_lock(&mCurrCMDMutex);
    mCurrCMD = new cMdmConnStat();
    stat = mCurrCMD->execute();
    delete mCurrCMD;
    mCurrCMD = 0;
    cyg_mutex_unlock(&mCurrCMDMutex);

    return stat;
}

bool cModem::link(char *address, int port)
{
	bool stat = false;
	cyg_uint8 rty = 0;

	do
	{
	    stat = IPstatus();

		cyg_thread_delay(500);

		if(mConnection == IPinitial)
		{
			cyg_mutex_lock(&mCurrCMDMutex);
			mCurrCMD = new cMdmStartUDP(address, port);
			stat = mCurrCMD->execute();
			delete mCurrCMD;
			mCurrCMD = 0;
			cyg_mutex_unlock(&mCurrCMDMutex);

			cyg_thread_delay(1000);

			//disconect when this fails
			if(!stat && mConnection != IPconnected)
			{
				if(shutIP())
					mConnection = IPclosed;
			}
		}
		else if(mConnection == IPgprsActive)
		{
			if(shutIP())
				mConnection = IPclosed;
		}

		cyg_thread_delay(500);

	}while(mConnection != IPconnected && rty++ < 20);

	if(mConnection == IPconnected)
		stat = true;

	return stat;
}

bool cModem::shutIP()
{
	bool stat;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmShut();
	stat = mCurrCMD->execute();
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

bool cModem::setFixedBaud()
{
	cyg_uint32 baud = 0;
	bool stat = false;

	{
		cyg_mutex_lock(&mCurrCMDMutex);
		mCurrCMD = new cMdmGetLocalBaud();
		if(mCurrCMD->execute())
			baud = ((cMdmGetLocalBaud*)mCurrCMD)->getBaud() ;

		delete mCurrCMD;
		mCurrCMD = 0;
		cyg_mutex_unlock(&mCurrCMDMutex);
	}
	if(baud == 115200)
		return true;

	{
		cyg_mutex_lock(&mCurrCMDMutex);
		mCurrCMD = new cMdmSetFixedBaud(115200);
		stat = mCurrCMD->execute();
		delete mCurrCMD;
		mCurrCMD = 0;
		cyg_mutex_unlock(&mCurrCMDMutex);
	}

	if(stat)
	{
		diag_printf(GREEN("BAUD fixed to 115.2kb/s\n"));
	}
	else
	{
		diag_printf(RED("Could not set BAUD\n_"));
	}

	return stat;
}

cyg_uint16 cModem::send(void* buff, cyg_uint16 len)
{
	//diag_printf("MDM: Try send %d\n", mConnection);
	if(mConnection != IPconnected)
		return 0;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmSend();
	len = ((cMdmSend*)(mCurrCMD))->send(buff, len);
	delete mCurrCMD;
	mCurrCMD = 0;
	cyg_mutex_unlock(&mCurrCMDMutex);

	return len;
}

void cModem::doCMD()
{
	diag_printf("Executing cmd sequence MODEM\n");
	if(!showID())
		return;

	if(!setEcho(0))
		return;

	if(!isSIMinserted())
	{
		diag_printf("Insert SIM\n");
		return;
	}


	if(getPINstat() == cMdmPINstat::SIM_PIN)
	{
		diag_printf("Enter PIN\n");
		if(!insertPIN("9842"))
		{
			diag_printf("incorrect PIN entered\n");
			return;
		}

		if(getPINstat() != cMdmPINstat::READY)
		{
			diag_printf("Invalid pin\n");
			return;
		}
	}

	int signalQ = -999;
	do
	{
		signalQ = getSQuality();
	}while(signalQ < -111);

	diag_printf("Signal %d\n", signalQ);

	bool registered = false;
	do
	{
		registered = isRegistered();
	}while(!registered);

	char netOperator[32];
	if(!getNetOperator(netOperator))
	{
		diag_printf("Could not get operator\n");
		return;
	}

	diag_printf("Network %s\n", netOperator);

	bool att = false;
	do
	{
		att = isGPRSattatched();
	}while(!att);

	if(!setGSMalphabet())
	{
		diag_printf("Could not set GSM alphabet\n");
		return;
	}


	if(!setIPstate(cMdmSetIPstate::SINGLE))
	{
		diag_printf("Could not set SINGLE connection state\n");
		return;
	}


	if(!setIPmode(cMdmSetIPmode::NORMAL))
	{
		diag_printf("Could not set NORMAL TCP application mode\n");
		return;
	}


	if(!setGPRSconnection("internet", "", ""))
	{
		diag_printf("Could not set GPRS connection details\n");
		return;
	}



	if(!setPort(cMdmSetLocalPort::UDP, 1000))
	{
		diag_printf("Could not set port details\n");
		return;
	}

	if(!setRXIPshow(true))
	{
		diag_printf("Could not enable RX IP show details\n");
		return;
	}


	//    if(!setTransparentCFG(3, 2, 256, true))
	//    {
	//    	diag_printf("Could not set Transparent config\n");
	//          	return;
	//    }

	if(!upConnection())
	{
		diag_printf("Could not bring the connection up\n");
		return;
	}




	char ip[32];
	if(!getLocalIP(ip))
	{
		diag_printf("Could not get Local ip\n");
		return;
	}

	diag_printf("local IP: %s\n", ip);


	char priDNS[32], secDNS[32];
	if(!getDNS(priDNS, secDNS))
	{
		diag_printf("Could not retrieve DNS\n");
		return;
	}

	diag_printf("primary DNS: %s\n", priDNS);
	diag_printf("secondary DNS: %s\n", secDNS);

	if(!disablePrompt())
	{
		diag_printf("Could not disable prompt\n");
		return;
	}

	if(!link("www.ostb.co.za", 7766))
	{
		diag_printf("Could not Link to network\n");
		return;
	}

	cyg_uint8 buff[20];
	int len = 20;
	for(int k = 0; k < len; k++)
		buff[k] = k;

	diag_printf("try send %d bytes\n", len);

	len = send(buff, len);

	if(len > 0)
	{
		diag_printf("Sent %d bytes\n", len);
	}


}

cyg_uint16 cModem::write(const char* str)
{
	cyg_uint32 len = strlen(str);
	dbg_printf(3, "cModemTX: (%d) - %s \n", len, str);

	Cyg_ErrNo err = cyg_io_write(mSerCMDHandle, str, &len);
	if(err < 0)
		diag_printf("cModemTXerr: %s", strerror(-err));

	return len;
}

cyg_uint16 cModem::write(void* buff, cyg_uint16 len)
{
	dbg_dump_buf(3, buff, len);

	Cyg_ErrNo err = cyg_io_write(mSerDataHandle, buff, (cyg_uint32*)&len);
	if(err < 0)
		diag_printf("cModemTXerr: %s", strerror(-err));

	return len;
}

void cModem::receive()
{
	//diag_printf("RECV data: ...\n");

	cyg_uint8 cnt = 0;
	do
	{
		mRXlen = 1;
		cyg_io_read(mSerDataHandle, mRXbuff, &mRXlen);

		if(mRXlen > 0 && mRXbuff[0] == '+')
			break;

	}while(cnt++ < 10);

	//compare for IDR
	char valid[] = {"IPD"};
	cnt = 0;
	int k = -1;
	do
	{
		k++;
		mRXlen = 1;
		cyg_io_read(mSerDataHandle, &mRXbuff[k], &mRXlen);

	}while(cnt++ < 10 && valid[k] == mRXbuff[k]);


	//now is RX byte count
	cnt = 0;
	k = -1;
	do
	{
		k++;
		mRXlen = 1;
		cyg_io_read(mSerDataHandle, &mRXbuff[k], &mRXlen);

	}while(cnt++ < 10 && mRXbuff[k] != ':');

	cyg_uint16 len = 0;
	if(k > 0)
	{
		mRXbuff[k] = 0;
		len = strtoul((char*)mRXbuff, NULL,10);
	}

	if(MODEM_BUFF_SIZE > len && len > 0)
	{
		mRXlen = len;
		cyg_io_read(mSerDataHandle, mRXbuff, &mRXlen);

		if(mRXlen > 0)
		{
			diag_printf("Modem: RXdata %d\n", mRXlen);
			diag_dump_buf(mRXbuff,mRXlen);

		}
	}
}

bool cModem::upModemLink(char *address, int port, char *apn, char* user_name, char* password)
{
	if(mModemStatus == Linked)
		return true;

	if(!shutIP())
		{
			diag_printf("Could not shut the current IP connection\n");
			return false;
		}


	if(!setGSMalphabet())
	{
		diag_printf("Could not set GSM alphabet\n");
		return false;
	}


	if(!setIPstate(cMdmSetIPstate::SINGLE))
	{
		diag_printf("Could not set SINGLE connection state\n");
		return false;
	}


	if(!setIPmode(cMdmSetIPmode::NORMAL))
	{
		diag_printf("Could not set NORMAL TCP application mode\n");
		return false;
	}


	if(!setGPRSconnection(apn, user_name, password))
	{
		diag_printf("Could not set GPRS connection details\n");
		return false;
	}



	if(!setPort(cMdmSetLocalPort::UDP, 1000))
	{
		diag_printf("Could not set port details\n");
		return false;
	}

	if(!setRXIPshow(true))
	{
		diag_printf("Could not enable RX IP show details\n");
		return false;
	}


	//    if(!setTransparentCFG(3, 2, 256, true))
	//    {
	//    	diag_printf("Could not set Transparent config\n");
	//          	return;
	//    }

	if(!upConnection())
	{
		diag_printf("Could not bring the connection up\n");
		return false;
	}

//	char ip[32];
//	if(!getLocalIP(ip))
//	{
//		diag_printf("Could not get Local ip\n");
//		return false;
//	}
//
//	diag_printf("local IP: %s\n", ip);
//
//
//	char priDNS[32], secDNS[32];
//	if(!getDNS(priDNS, secDNS))
//	{
//		diag_printf("Could not retrieve DNS\n");
//		return false;
//	}
//
//	diag_printf("primary DNS: %s\n", priDNS);
//	diag_printf("secondary DNS: %s\n", secDNS);

	if(!disablePrompt())
	{
		diag_printf("Could not disable prompt\n");
		return false;
	}

	if(!link(address, port))
	{
		diag_printf("Could not Link to network\n");
		return false;
	}

	mModemStatus = Linked;
	return true;
}

void cModem::debug(cTerm & term, int argc,char * argv[])
{
	if(!_instance)
		return;

	if(!strcmp(argv[0], "modemUp"))
	{
		_instance->updateStatus();
		if(_instance->mModemStatus == SIMnotReady)
			_instance->showSIMStatus(_instance->mSIMstatus);
		else
			_instance->showModemStatus(_instance->mModemStatus);
	}
	else if(!strcmp(argv[0], "modemStat"))
	{
		if(_instance->mModemStatus == SIMnotReady)
			_instance->showSIMStatus(_instance->mSIMstatus);
		else
			_instance->showModemStatus(_instance->mModemStatus);

	}
	else if(!strcmp(argv[0], "pb"))
	{
		if(argc > 3)
		{
			int idx = atoi(argv[1]);
			diag_printf("Update phonebook[%d] to %s - %s\n", idx, argv[2], argv[3]);
			_instance->updatePhoneBook(idx, argv[2], argv[3]);
		}
		else if(argc > 1)
		{
			int idx = atoi(argv[1]);
			diag_printf("Remove phonebook[%d]\n", idx);
			_instance->updatePhoneBook(idx, 0, 0);
		}

		_instance->listPhoneBook();
	}
	else if(!strcmp(argv[0], "sms"))
	{
		if(argc > 1)
		{
			if( _instance->sendSMS(argv[1], "Test\nSIM900"))
				diag_printf("SMS Sent\n");
			else
				diag_printf("SMS fail\n");
		}
		else
		{
			diag_printf("Input a number to sms to\n");
		}
	}
	else if(!strcmp(argv[0], "rsms"))
	{
		_instance->readSMS();
	}
	else if(!strcmp(argv[0], "bal"))
	{
		_instance->checkBalance();
	}
}

void cModem::updatePhoneBook(int idx, const char* name, const char* number)
{
	if(name && number)
	{
		mCurrCMD = new cMdmUpdatePB(idx, name, number);
		mCurrCMD->execute();
		delete mCurrCMD;
		mCurrCMD = 0;
	}
	else
	{
		mCurrCMD = new cMdmUpdatePB(idx);
		mCurrCMD->execute();
		delete mCurrCMD;
		mCurrCMD = 0;
	}
}

void cModem::listPhoneBook()
{
	cMdmGetPB::s_entry list[20];
	int pbSize = 20;
	int pbCount = 0;

	if(getPhoneBook(list, &pbCount, &pbSize))
	{

		diag_printf("SIM phonebook (%d:%d)\n", pbCount, pbSize);

		diag_printf(UNDERLINE("%3s%16s%20s\n"), "Idx", "Name", "Number");
		for(int k = 0; k < pbCount; k++)
		{
			diag_printf("%3d%16s%20s\n", k + 1, list[k].name, list[k].number);
		}
	}

}

bool cModem::getPhoneBook(cMdmGetPB::s_entry* list, int* count, int* size)
{
	int pbCount = 0;
	int pbSize = 0;
	bool stat = false;

	cyg_mutex_lock(&mCurrCMDMutex);
	mCurrCMD = new cMdmGetPBCount();
	if(mCurrCMD->execute())
	{
		stat = true;
		pbCount = ((cMdmGetPBCount*)mCurrCMD)->count();
		pbSize = ((cMdmGetPBCount*)mCurrCMD)->size();
	}
	delete mCurrCMD;
	mCurrCMD = 0;


	if(!pbCount || *size < pbCount)
	{
		*size = -1;
		stat =  false;
	}

	if(stat)
	{
		mCurrCMD = new cMdmGetPB(list, pbCount);
		stat = mCurrCMD->execute();
		delete mCurrCMD;
		mCurrCMD = 0;

		*count = pbCount;
		*size = pbSize;
	}
	cyg_mutex_unlock(&mCurrCMDMutex);

	return stat;
}

void cModem::readSMS()
{
	cMdmReadSMS::sSMS * list[10];
	for(cyg_uint8 k = 0; k < 10; k++)
		list[k] = 0;

	bool stat = false;
	mCurrCMD = new cMdmReadSMS(list);
	stat = mCurrCMD->execute();
	delete mCurrCMD;
	mCurrCMD = 0;

	if(stat)
	{
		diag_printf("MODEM: SMS success\n");

		cyg_uint8 k = 0;
		while(list[k])
		{
			list[k]->show();
			delete list[k];

			k++;
		}
	}
}

void cModem::ATcmd(cTerm & term, int argc,char * argv[])
{
	if(!_instance)
		return;

	char atCMD[32];
	sprintf(atCMD,"%s\n", argv[0]);

	_instance->write(atCMD);
}

cModem::~cModem()
{
}

