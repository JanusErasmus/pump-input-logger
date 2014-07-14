#include <cyg/kernel/diag.h>
#include <cyg/io/ttyio.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "modem.h"

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
	mCurrCMD = 0;
	mStat = Off;
	mReg = notRegistered;
	mConnection = initial;
	mSIM = SIMpin;

	// Modem CMD UART
	Cyg_ErrNo err =	cyg_io_lookup(serDev,&mSerCMDHandle);

	diag_printf("cModem CMD %p: %s \n", mSerCMDHandle, strerror(-err));

	cyg_serial_info_t info;

	info.flags = 0;
	info.baud = CYGNUM_SERIAL_BAUD_115200;
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

	cyg_uint32 reg32;
	HAL_READ_UINT32(CYGHWR_HAL_STM32_UART2 + CYGHWR_HAL_STM32_UART_BRR, reg32);
	diag_printf("\t BRR: 0x%08X\n", reg32);
//	reg32 = 0x24;
//	HAL_WRITE_UINT32(CYGHWR_HAL_STM32_UART2 + CYGHWR_HAL_STM32_UART_BRR, reg32);

	cyg_thread_resume(mThreadHandle);

}

cModem::eModemStat cModem::getStatus()
{
	//check if modem is off
	if(!showID())
	{
		//reset all statuses
		mStat = Off;
		mSIM = SIMneeded;
		mReg = notRegistered;
		mConnection = initial;
		return Off;
	}

	if(mReg == registered && mConnection == connected)
	{
		mStat = Linked;
		return Linked;
	}

	if(mReg == registered && mStat == GPRSatt)
	{
		return GPRSatt;
	}
	else
	{
		mStat = NetworkBusy;
	}


	//check SIM status
	if(mSIM != SIMready)
	{
		mSIM = simStatus();
		switch (mSIM)
		{
			case SIMneeded:
				return needSIM;
			case SIMpin:
				return needPIN;
				break;
			case SIMpuk:
				return needPUK;
			case SIMready:
				if(isRegistered())
				{
					mReg = registered;
					break;
				}
				else
				{
					return NetworkBusy;
				}

			default:
				break;
		}
	}

	//check registration
	if(mReg != registered)
	{
		//check network registration
		if(isRegistered())
			mStat = NetworkReg;
		else
			return NetworkBusy;
	}

	//attach GPRS
	if(mStat != GPRSatt)
	{
		if(!isGPRSattatched())
			return NetworkReg;

		mStat = GPRSatt;
	}

	return mStat;
}

cModem::eSIMstat cModem::simStatus()
{
	if(!isSIMinserted())
		return SIMneeded;

	cMdmPINstat::ePINstat pinStatus = getPINstat();
	switch(pinStatus)
	{
		case cMdmPINstat::SIM_PIN:
			return SIMpin;
		case cMdmPINstat::SIM_PUK:
			return SIMpuk;
		case cMdmPINstat::READY:
			return SIMready;
		default:
			return SIMneeded;
	}
}


void cModem::showStatus(eModemStat stat)
{
	switch(stat)
	{
		case Off:
			diag_printf("cModemStat: OFF\n");
			break;
		case needSIM:
			diag_printf("cModemStat: Insert SIM\n");
			break;
		case needPIN:
			diag_printf("cModemStat: insert PIN\n");
			break;
		case needPUK:
			diag_printf("cModemStat: insert PUK\n");
			break;
		case NetworkBusy:
			diag_printf("cModemStat: Network Busy\n");
			break;
		case NetworkReg:
			diag_printf("cModemStat: Network registered\n");
			break;
		case GPRSatt:
			diag_printf("cModemStat: GPRS attatched\n");
			break;
		case Linked:
			diag_printf("cModemStat: LINKED\n");
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


	mRXlen = 256;
	Cyg_ErrNo err = cyg_io_read(mSerCMDHandle, mRXbuff, &mRXlen);
	if(err < 0)
		diag_printf("cModemErr: %s \n", strerror(-err));

	if(mRXlen > 0 && mRXlen < 256)
	{
		if(mRXbuff[mRXlen] != 0)
			mRXbuff[mRXlen] = 0;

		if(mDebugLevel >= 3)
		{
//			diag_printf("RX: %d\n", mRXlen);
//			diag_dump_buf(mRXbuff, mRXlen);
			diag_printf("%s", mRXbuff);
		}

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
	char buff[256];
	strcpy(buff, (char*) response);
	int cmdLen;

	cmdLen = 6;
	if(!strncmp(buff,"+CREG:", cmdLen))
	{
		switch(strtoul(&buff[cmdLen],0,10) == 1)
		{
			case 0:
				diag_printf("CREG: NOT REGISTERED\n");
				mReg = notRegistered;
				mConnection = initial;
				break;
			case 1:
				diag_printf("CREG: REGISTERED\n");
				mReg = registered;
				break;
			case 2:
				diag_printf("CREG: SEARCHING\n");
				mReg = searching;
				mConnection = initial;
				break;
			case 3:
				diag_printf("CREG: DENIED\n");
				mReg = denied;
				break;
			case 4:
				diag_printf("CREG: UNKNOWN\n");
				mReg = unknown;
				break;
			case 5:
				diag_printf("CREG: ROAMIMG\n");
				mReg = roaming;
				break;
			default:
				break;
		}
	}


	cmdLen = 7;
	if(!strncmp(buff,"STATE: ",cmdLen))
	{
		//printf("STAT response: %s\n", &response[cmdLen]);

		if(!strcmp(&buff[cmdLen], "IP STATUS\r"))
		{
			mConnection = initial;
			dbg_printf(1, "IPCONN: STATUS\n");
		}
		else if(!strcmp(&buff[cmdLen], "IP INITIAL\r"))
		{
			mConnection = initial;
			diag_printf("IPCONN: IP INITIAL\n");
		}
		else if(!strcmp(&buff[cmdLen], "UDP CONNECTING\r"))
		{
			mConnection = connecting;
			diag_printf("IPCONN: CONNECTING\n");
		}
		else if(!strcmp(&buff[cmdLen], "CONNECT OK\r"))
		{
			mConnection = connected;
			diag_printf("IPCONN: CONNECTED\n");
		}
		else if(!strcmp(&buff[cmdLen], "IP GPRSACT\r"))
		{
			mConnection = gprsActive;
			diag_printf("IPCONN: GPRS ACTIVE\n");
		}
		else if(!strcmp(&buff[cmdLen], "CONNECTED\r"))
		{
			mConnection = connected;
			diag_printf("IPCONN: CONNECTED\n");
		}
		else if(!strcmp(&buff[cmdLen], "UDP CLOSED\r"))
		{
			mConnection = closed;
			diag_printf("IPCONN: CLOSED\n");
		}
	}


	if(!strcmp(buff, "CONNECT OK\r"))
	{
		mConnection = connected;
		diag_printf("IPCONN: CONNECTED\n");
	}

	if(!strcmp(buff, "ALREADY CONNECT\r"))
	{
		mConnection = connected;
		diag_printf("IPCONN: CONNECTEDd\n");
	}

	if(!strcmp(buff, "SHUT OK\r"))
	{
		mConnection = closed;
		diag_printf("IPCONN: CLOSED\n");
	}

	if(!strcmp(buff, "CLOSE OK\r"))
	{
		mConnection = closed;
		diag_printf("IPCONN: CLOSED\n");
	}

	cmdLen = 4;
	if(!strncmp(buff,"RECV", cmdLen) && mConnection == connected)
	{
		receive();
	}
}

void cModem::power(bool stat)
{
	if(stat)
	{
		set_pinH(MDM_PWR_OFF);
	}

	if((!stat && showID()) || (stat && !showID()))
	{
		set_pinH(SIMCOM_PWR_KEY);
		cyg_thread_delay(600);
		set_pinL(SIMCOM_PWR_KEY);
	}
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

	mCurrCMD = new cMdmGetID();
	if(mCurrCMD->execute())
	{
		diag_printf("Modem ID: %s\n", ((cMdmGetID*)(mCurrCMD))->getID());
		stat  =true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;

	return stat;
}



bool cModem::setEcho(bool stat)
{
	mCurrCMD = new cMdmSetEcho(stat);
	stat = false;

	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;

	return stat;
}

bool cModem::isSIMinserted()
{
	bool simStat = false;
	mCurrCMD = new cMdmSIMinserted();
	if(mCurrCMD->execute())
	{
		if(((cMdmSIMinserted*)(mCurrCMD))->inserted()){
			simStat = true;
		}
	}
	delete mCurrCMD;
	mCurrCMD = 0;

	return simStat;
}

cMdmPINstat::ePINstat cModem::getPINstat()
{
	cMdmPINstat::ePINstat stat = cMdmPINstat::UNKNOWN;
	mCurrCMD = new cMdmPINstat();
	if(mCurrCMD->execute())
	{
		stat = ((cMdmPINstat*)(mCurrCMD))->stat();
	}
	delete mCurrCMD;
	mCurrCMD = 0;

	return stat;
}

bool cModem::insertPIN(char* pin)
{
	bool stat = false;
	mCurrCMD = new cMdmEnterPIN(pin);
	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;

	return stat;
}

bool cModem::insertPUK(char* puk, char* pin)
{
	bool stat = false;
	mCurrCMD = new cMdmEnterPIN(puk, pin);
	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;

	return stat;
}


int cModem::getSQuality()
{
	int signalQ = -999;
	mCurrCMD = new cMdmSignalQ();
	if(mCurrCMD->execute())
	{
		signalQ = ((cMdmSignalQ*)(mCurrCMD))->strength();
	}
	delete mCurrCMD;
	mCurrCMD = 0;

	return signalQ;
}

bool cModem::isRegistered()
{
	bool stat = false;
	mCurrCMD = new cMdmNetRegistration();
	if(mCurrCMD->execute())
	{
		stat = ((cMdmNetRegistration*)(mCurrCMD))->stat();
	}
	delete mCurrCMD;
	mCurrCMD = 0;

	return stat;
}

bool cModem::getNetOperator(char* netOperator)
{
	bool stat = false;

	mCurrCMD = new cMdmNetOperator();
	if(mCurrCMD->execute())
	{
		strcpy(netOperator, ((cMdmNetOperator*)(mCurrCMD))->name());
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;

	return stat;
}

bool cModem::isGPRSattatched()
{
	bool stat = false;;

	mCurrCMD = new cMdmGPRSattched();
	if(mCurrCMD->execute())
	{
		stat = ((cMdmGPRSattched*)((mCurrCMD)))->stat();
	}
	delete mCurrCMD;
	mCurrCMD = 0;

	return stat;
}

bool cModem::setGSMalphabet()
{
	bool stat = false;

	mCurrCMD = new cMdmSetGSMalphabet();
	if(mCurrCMD->execute()){
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	return stat;
}

bool cModem::setIPstate(cMdmSetIPstate::IPconn conStat)
{
	bool stat = false;

	mCurrCMD = new cMdmSetIPstate();
	//set state
	((cMdmSetIPstate*)(mCurrCMD))->state(conStat);

	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;

	return stat;
}

bool cModem::setIPmode(cMdmSetIPmode::IPmode mode)
{
	bool stat = false;

	mCurrCMD = new cMdmSetIPmode();
	//set state
	((cMdmSetIPmode*)((mCurrCMD)))->mode(mode);
	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;
	return stat;
}

bool cModem::setGPRSconnection(char *apn, char* user_name, char* password)
{
	bool stat = false;

	mCurrCMD = new cMdmSetGPRSconnection(apn, user_name, password);
	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;

	return stat;
}

bool cModem::setPort(cMdmSetLocalPort::Lmode mode, cyg_uint16 port)
{
	bool stat = false;

	mCurrCMD = new cMdmSetLocalPort();

	((cMdmSetLocalPort*)(mCurrCMD))->setPort(mode, port);

	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;

	return stat;
}

bool cModem::setRXIPshow(bool shown)
{
	bool stat = false;

	mCurrCMD = new cMdmSetRXIPshow(shown);
	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;

	return stat;
}

bool cModem::setTransparentCFG(int retryN, int waitN, int buffLen, bool escSeq)
{
	bool stat = false;

	mCurrCMD = new cMdmCfgTransparentTx(retryN, waitN, buffLen, escSeq);
	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;

	return stat;
}

bool cModem::upConnection()
{
	bool stat = false;

	while(!stat){
		//try start task
		mCurrCMD = new cMdmStartTask();
		stat = mCurrCMD->execute();
		delete mCurrCMD;
		mCurrCMD = 0;
		//Couldn't start task, shut down all IP connections
		if(!stat)
		{
			if(shutIP())
				mConnection = closed;
		}
	}

	mCurrCMD = new cMdmUpConnection();
	stat = mCurrCMD->execute();
	delete mCurrCMD;
	mCurrCMD = 0;
	return stat;
}

bool cModem::getLocalIP(char *ip)
{
	bool stat = false;

	mCurrCMD = new cMdmGetLocalIP();
	if(mCurrCMD->execute())
	{
		((cMdmGetLocalIP*)(mCurrCMD))->setIP(ip);
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;

	return stat;
}

bool cModem::getDNS(char *priDNS, char *secDNS)
{
	bool stat = false;

	mCurrCMD = new cMdmCfgDNS();
	if(mCurrCMD->execute())
	{
		((cMdmCfgDNS*)((mCurrCMD)))->getDNS(priDNS, secDNS);
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;

	return stat;
}

bool cModem::disablePrompt()
{
	bool stat = false;

	mCurrCMD = new cMdmSetPrompt(cMdmSetPrompt::noPrompt);
	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;

	if(!stat)
		return false;

	mCurrCMD = new cMdmSetRXHeader(cMdmSetRXHeader::header);
	if(mCurrCMD->execute())
	{
		stat = true;
	}
	delete mCurrCMD;
	mCurrCMD = 0;

	return stat;
}

bool cModem::IPstatus()
{
	bool stat;

    mCurrCMD = new cMdmConnStat();
    stat = mCurrCMD->execute();
    delete mCurrCMD;
    mCurrCMD = 0;

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

		if(mConnection == initial)
		{
			mCurrCMD = new cMdmStartUDP(address, port);
			stat = mCurrCMD->execute();
			delete mCurrCMD;
			mCurrCMD = 0;

			cyg_thread_delay(1000);

			//disconect when this fails
			if(!stat && mConnection != connected)
			{
				if(shutIP())
					mConnection = closed;
			}
		}
		else if(mConnection == gprsActive)
		{
			if(shutIP())
				mConnection = closed;
		}

		cyg_thread_delay(500);

	}while(mConnection != connected && rty++ < 20);

	if(mConnection == connected)
		stat = true;

	return stat;
}

bool cModem::shutIP()
{
	bool stat;
	mCurrCMD = new cMdmShut();
	stat = mCurrCMD->execute();
	delete mCurrCMD;
	mCurrCMD = 0;

	return stat;
}

bool cModem::setFixedBaud()
{
	cyg_uint32 baud = 0;
	bool stat = false;

	{
		mCurrCMD = new cMdmGetLocalBaud();

		if(mCurrCMD->execute())
			baud = ((cMdmGetLocalBaud*)mCurrCMD)->getBaud() ;

		delete mCurrCMD;
		mCurrCMD = 0;
	}
	if(baud == 115200)
		return true;

	{
		mCurrCMD = new cMdmSetFixedBaud(115200);
		stat = mCurrCMD->execute();
		delete mCurrCMD;
		mCurrCMD = 0;
	}

	if(stat)
	{
		dbg_printf(green, "BAUD fixed to 115.2kb/s\n");
	}
	else
	{
		dbg_printf(red, "Could not set BAUD\n");
	}

	return stat;
}

cyg_uint16 cModem::send(void* buff, cyg_uint16 len)
{
	//diag_printf("MDM: Try send %d\n", mConnection);
	if(mConnection != connected)
		return 0;

	mCurrCMD = new cMdmSend();
	len = ((cMdmSend*)(mCurrCMD))->send(buff, len);
	delete mCurrCMD;
	mCurrCMD = 0;

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
			//diag_printf("Modem: RX: %d\n", mRXlen);
			//diag_dump_buf(mRXbuff,mRXlen);
			cSysMon::get()->dataReceived(mRXbuff, mRXlen);
		}
	}
}

bool cModem::upModemLink(char *address, int port, char *apn, char* user_name, char* password)
{
	if(mStat == Linked)
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

	mStat = Linked;
	return true;
}

cModem::~cModem()
{
}

