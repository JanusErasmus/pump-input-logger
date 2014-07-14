#include <cyg/kernel/diag.h>
#include <stdio.h>
#include <stdlib.h>

#include "mdm_cmd.h"
#include "modem.h"
#include "utils.h"

cMdm_cmd::cMdm_cmd( const char * fmt,...)
{
	va_list vl;
	va_start(vl,fmt);
	vsnprintf(mTempStr, 64, fmt,vl);
	va_end(vl);
	mCMD = mTempStr;

	//printf("CMD: %s\n",mCMD);
	cyg_thread_delay(150);

	replied = false;
	cyg_mutex_init(&mWaitMutex);
	cyg_cond_init(&mWaitCond, &mWaitMutex);

	cyg_mutex_init(&mBuffMutex);
}

cyg_uint8* cMdm_cmd::getCMD()
{
	return (cyg_uint8*)mCMD;
}

bool cMdm_cmd::waitReply()
{
	if(!replied)
	{
		cyg_uint32 timer = cyg_current_time();
		//diag_printf(".");
		cyg_cond_timed_wait(&mWaitCond, timer + 200);
	}

	return replied;
}

bool cMdm_cmd::execute()
{
	bool stat = false;
	int cmdCnt = 0;

	//diag_printf("sending CMD: %d: %s \n", strlen(mCMD), mCMD);
	replied = false;
	cModem::get()->write(mCMD);

	do
	{
		//printf("cmdC: %d\n", cmdCnt);
		if(waitReply())
		{
			//diag_printf("cmdX: %s\n", (char*) mRxBuff);

			cmdCnt = 0;  //got reply so handle it
			if(mRxLen > 0)
			{
				switch(getResponse())
				{
					case ok:
						//diag_printf("+");
						stat =  true;
						cmdCnt = 10;
						break;

					case err:
						//diag_printf("-");
						stat = false;
						cmdCnt = 10;
						break;

					default:
						if(handleResponse((const char*)mRxBuff))
						{
							stat = true;
							cmdCnt = 10;
						}
						break;
				}

				replied = false;
			}
		}
	}while(cmdCnt++ < 3);

	if(cmdCnt == 10)
	{
		diag_printf("Timed out\n");
	}

	return stat;
}

void cMdm_cmd::handleData(cyg_uint8* data, int len)
{
	while(replied)
		cyg_thread_delay(10);

	//diag_printf("cmdH: %s\n", data);

	memcpy(mRxBuff, data, len);
	mRxBuff[len] = 0;
	mRxLen = len;

	replied = true;
	cyg_cond_signal(&mWaitCond);
}

cMdm_cmd::eMdm_response cMdm_cmd::getResponse()
{
	if(!strcmp((char*)mRxBuff, "OK\r"))
		return ok;
	else if(!strcmp((char*)mRxBuff, "ERROR\r"))
		return err;

	return unknown;
}

bool cMdm_cmd::handleResponse(const char* response)
{
	//printf("Unknown data: %s\n", response);
	return false;
}

bool cMdm_cmd::parseValue(const char* buff, char delimiter, int* paramC, char paramV[][32])
{
	bool stat = false;
	int start = 0, end = 0;
	int found  = 0;
	int len = strlen(buff);

	do
	{
		//search for delimiter
		for( int k = start; k < len; k++)
		{
			if(buff[k] == delimiter)
			{
				end = k;
				break;
			}
		}

		if(start == end + 1)
			end = len;

		//printf("found delim %d\n", end);
		memcpy(paramV[found], &buff[start], end - start);
		paramV[found][end - start] = 0;

		start = end + 1;

		//printf("param%d: %s\n", found, paramV[found]);
		found++;

	}while(end < len);

	*paramC = found;

	return stat;
}

void cMdm_cmd::setCMD(char* cmd)
{
	strcpy(mTempStr, cmd);
	mCMD = mTempStr;
}

int cMdm_cmd::waitData(char* buff)
{
	int len = 0;

	if(waitReply())
	{
		memcpy(buff, mRxBuff, mRxLen);
		len = mRxLen;

		mRxBuff[len] = 0;
		mRxLen = len;

		replied = false;
		cyg_cond_signal(&mWaitCond);
	}

	return len;
}

cMdm_cmd::~cMdm_cmd()
{
}


cMdmGetID::cMdmGetID() : cMdm_cmd("ATI\n")
{
	id[0] = 0;
}


bool cMdmGetID::handleResponse(const char* response)
{
	char cmdReply[] = {"SIM900"};

	//diag_printf("ID:\n %s\n - %s\n", response, cmdReply);
	if(!strncmp(cmdReply, response, 6))
	{
		strcpy(id, &response[7]);
	}
	return false;

}

cMdmSetEcho::cMdmSetEcho(bool stat)
	: cMdm_cmd("ATE%d\n", stat)
{
}

cMdmSIMinserted::cMdmSIMinserted() : cMdm_cmd("AT+CSMINS?\n")
{
	mInserted = false;
}


bool cMdmSIMinserted::handleResponse(const char* response)
{
	char cmdReply[] = {"+CSMINS:"};
	int cmdLen = strlen(cmdReply);

	//printf("SIM - %s\n", response );

	if(!strncmp(cmdReply,response, cmdLen))
	{
		char *argv[3];
		int argc = 3;
		util_parse_params((char*)&response[cmdLen],argv,argc,',',',');

		if(argc >= 1)
		{
			//parameter 1 is the sim status
			if(strtoul(argv[1],0,10) == 1)
				mInserted = true;
		}
	}

	return false;
}

cMdmPINstat::cMdmPINstat() : cMdm_cmd("AT+CPIN?\n")
{
	mStat = UNKNOWN;
}


bool cMdmPINstat::handleResponse(const char* response)
{
	char cmdReply[] = {"+CPIN:"};
	int cmdLen = strlen(cmdReply);

	//printf("PIN - %s\n", response);

	if(!strncmp(cmdReply,response, cmdLen))
	{
		//printf("PIN stat: %s\n", &response[cmdLen]);
		if(!strcmp(&response[cmdLen], " READY\r"))
		{
			mStat = READY;
		}
		else if(!strcmp(&response[cmdLen], " SIM PIN\r"))
		{
			mStat = SIM_PIN;
		}
		else if(!strcmp(&response[cmdLen], " SIM PUK\r"))
		{
			mStat = SIM_PUK;
		}
		else if(!strcmp(&response[cmdLen], " PH_SIM PIN\r"))
		{
			mStat = PH_SIM_PIN;
		}
		else if(!strcmp(&response[cmdLen], " PH_SIM PUK\r"))
		{
			mStat = PH_SIM_PUK;
		}
	}

	return false;
}

cMdmEnterPIN::cMdmEnterPIN(char* pin) : cMdm_cmd("AT+CPIN=%s\n", pin)
{
}

cMdmEnterPIN::cMdmEnterPIN(char* puk, char* pin) : cMdm_cmd("AT+CPIN=%s,%s\n",puk, pin)
{
}

cMdmSignalQ::cMdmSignalQ() : cMdm_cmd("AT+CSQ\n")
{
	mRSSI = -999;
}

bool cMdmSignalQ::handleResponse(const char* response)
{
	char cmdReply[] = {"+CSQ:"};
	int cmdLen = strlen(cmdReply);

	//printf("%s - %s\n", response, statReply);

	if(!strncmp(cmdReply, response, cmdLen))
	{
		//printf("Signal stat: %s\n", &response[cmdLen]);

//		char param[3][32];
//		int cnt;
//		parseValue(&response[cmdLen],',',&cnt,param);
		char *argv[3];
		int argc = 3;
		util_parse_params((char*)&response[cmdLen],argv,argc,',',',');

		if(argc >= 1)
		{
			//parameter 0 is the rssi
			int level = strtoul(argv[0],0,10);
			if(level == 0)
				mRSSI = -115;

			if (level == 1)
				mRSSI =  -111;

			if(level > 1 && level < 30)
				mRSSI = (int)( (0.509 * (float)level) - 55.5 );

			if(level == 30)
				mRSSI = -52;
		}
	}

	return false;
}

cMdmNetOperator::cMdmNetOperator() : cMdm_cmd("AT+COPS?\n")
{
	mOperator[0] = 0;
}

bool cMdmNetOperator::handleResponse(const char* response)
{
	char cmdReply[] = {"+COPS:"};
	int cmdLen = strlen(cmdReply);

	//printf("%s - %s\n", response, statReply);

	if(!strncmp(cmdReply, response, cmdLen))
	{
		//printf("response: %s\n", &response[cmdLen]);

		char *argv[3];
		int argc = 3;
		util_parse_params((char*)&response[cmdLen],argv,argc,',',',');

		if(argc > 2)
		{
			//parameter 2 is the selected network name
			int len = strlen(argv[2]);
			memcpy(mOperator,&argv[2][1], len - 3);
			mOperator[len - 3] = 0;
		}
	}

	return false;
}

cMdmNetRegistration::cMdmNetRegistration() : cMdm_cmd("AT+CREG?\n")
{
	mStat = false;
}

bool cMdmNetRegistration::handleResponse(const char* response)
{
	char cmdReply[] = {"+CREG:"};
	int cmdLen = strlen(cmdReply);


	if(!strncmp(cmdReply,response,cmdLen))
	{
		//printf("response: %s\n", &response[cmdLen]);

		char *argv[3];
		int argc = 3;
		util_parse_params((char*)&response[cmdLen],argv,argc,',',',');

		if(argc > 1)
		{
			//parameter 0 is URC code enabled flag

			//parameter 1 is registration flag
			if(strtoul(argv[1],0,10) == 1)
			{
				mStat = true;
			}
		}
	}

	return false;
}

cMdmGPRSattched::cMdmGPRSattched() : cMdm_cmd("AT+CGATT?\n")
{
	mStat = false;
}

bool cMdmGPRSattched::handleResponse(const char* response)
{
	char cmdReply[] = {"+CGATT:"};
	int cmdLen = strlen(cmdReply);


	if(!strncmp(cmdReply,response,cmdLen))
	{
		//printf("response: %s\n", &response[cmdLen]);

		char *argv[3];
		int argc = 3;
		util_parse_params((char*)&response[cmdLen],argv,argc,',',',');

		if(argc > 0)
		{
			//parameter 1 is registration flag
			if(strtoul(argv[0],0,10) == 1)
			{
				mStat = true;
			}
		}
	}

	return false;
}

cMdmSetGSMalphabet::cMdmSetGSMalphabet() : cMdm_cmd("AT+CSCS=\"GSM\"\n")
{
}

cMdmSetIPstate::cMdmSetIPstate() : cMdm_cmd("AT+CIPMUX?\n")
{
	mIPstate = UNKNOWN;
}

void cMdmSetIPstate::state(IPconn stat)
{
	char txt[32];
	sprintf(txt,"AT+CIPMUX=%d\r\n",stat);
	setCMD(txt);

}

bool cMdmSetIPstate::handleResponse(const char* response)
{
	char cmdReply[] = {"+CIPMUX: "};
	int cmdLen = strlen(cmdReply);

	//printf("State response: %s\n", response);

	if(!strncmp(cmdReply,response,cmdLen))
	{
		//printf("\tMUX: %s\n", &response[cmdLen]);

		//printf("IP:%d\n", (int)strtol((char*)&response[cmdLen],0,10));
		if(strtoul(&response[cmdLen],0,10) == 0)
		{
			//printf("IP: SINGLE\n");
			mIPstate = SINGLE;
		}

		if(strtoul(&response[cmdLen],0,10) == 1)
		{
			//printf("IP: MULTI\n");
			mIPstate = MULTI;
		}
	}

	return false;
}

cMdmSetIPmode::cMdmSetIPmode() : cMdm_cmd("AT+CIPMODE?\n")
{
	mIPmode = UNKNOWN;
}

void cMdmSetIPmode::mode(IPmode mod)
{
	char txt[32];
	sprintf(txt,"AT+CIPMODE=%d\n",mod);
	setCMD(txt);

}

bool cMdmSetIPmode::handleResponse(const char* response)
{
	char cmdReply[] = {"+CIPMODE:"};
	int cmdLen = strlen(cmdReply);


	if(!strncmp(cmdReply,response,cmdLen))
	{
		//printf("response: %s\n", &response[cmdLen]);

		//printf("IP:%d\n", (int)strtol((char*)&response[cmdLen],0,10));
		if(strtoul(&response[cmdLen],0,10) == 0)
		{
			mIPmode = NORMAL;
		}

		if(strtoul(&response[cmdLen],0,10) == 1)
		{
			mIPmode = TRANS;
		}
	}

	return false;
}


cMdmSetGPRSconnection::cMdmSetGPRSconnection(char* apn, char* user_name, char* password)
	: cMdm_cmd("AT+CIPCSGP=1,\"%s\",\"%s\",\"%s\"\n", apn, user_name, password)
{
}

cMdmSetLocalPort::cMdmSetLocalPort() : cMdm_cmd("AT+CLPORT?\n")
{
	mPmode = UNKNOWN;
	mTCPport = -1;
	mUDPport = -1;
}

void cMdmSetLocalPort::setPort(Lmode mod, cyg_uint16 port)
{
	char txt[32];

	switch(mod)
	{
		case TCP:
		sprintf(txt,"AT+CLPORT=\"TCP\",%d\n", port);
		break;

		case UDP:
			sprintf(txt,"AT+CLPORT=\"UDP\",%d\n", port);
			break;

		default:
			sprintf(txt,"AT+CLPORT?\n");
			break;
	}
	setCMD(txt);

}

bool cMdmSetLocalPort::handleResponse(const char* response)
{
	char cmdReply[16];
	int cmdLen;

	strcpy(cmdReply,"TCP:");
	cmdLen = strlen(cmdReply);
	if(!strncmp(cmdReply,response,cmdLen))
	{
		//printf("TCP response: %s\n", &response[cmdLen]);
		mTCPport = strtoul(&response[cmdLen], 0, 10);
		return true;
	}

	strcpy(cmdReply, "UDP:");
	cmdLen = strlen(cmdReply);
	if(!strncmp(cmdReply,response,cmdLen))
	{
		//printf("UDP response: %s\n", &response[cmdLen]);
		mUDPport = strtoul(&response[cmdLen], 0, 10);
		return true;
	}


	return false;
}

cyg_uint16 cMdmSetLocalPort::getPort(Lmode mode)
{
	switch(mode)
	{
		case TCP:
			return mTCPport;

		case UDP:
			return mUDPport;

		default:
			return -1;
	}
}

cMdmSetRXIPshow::cMdmSetRXIPshow(bool stat)
	: cMdm_cmd("AT+CIPSRIP=%d\n", stat)
{
}


cMdmCfgTransparentTx::cMdmCfgTransparentTx(int retryN, int waitN, int buffLen, bool escSeq)
	: cMdm_cmd("AT+CIPCCFG=%d,%d,%d,%d\n", retryN, waitN, buffLen, escSeq)
{
}


cMdmStartTask::cMdmStartTask()
	: cMdm_cmd( "AT+CSTT\n")
{
}


cMdmUpConnection::cMdmUpConnection()
	: cMdm_cmd("AT+CIICR\n")
{
}


cMdmGetLocalIP::cMdmGetLocalIP()
	: cMdm_cmd("AT+CIFSR\n")
{
}

bool cMdmGetLocalIP::handleResponse(const char* response)
{
	bool stat = false;

	if(isIP(response))
	{
		//printf("IP: %s\n", response);
		strcpy(mIP, response);

		stat = true;
	}
	return stat;
}

bool cMdmGetLocalIP::isIP(const char* buff)
{
	char *argv[4];
	int argc = 4;
	util_parse_params((char*)buff,argv,argc,'.','.');

	//printf("CNT is: %d", cnt);

	if(argc > 3)
	{
		return true;
	}

	return false;
}

void cMdmGetLocalIP::setIP(char* ip)
{
	strcpy(ip, mIP);
}


cMdmCfgDNS::cMdmCfgDNS() : cMdm_cmd("AT+CDNSCFG?\n")
{
	mPrimary[0] = 0;
	mSecondary[0] = 0;
}

void cMdmCfgDNS::setDNS(char* pri_dns, char* sec_dns)
{
	char txt[32];

	sprintf(txt,"AT+CDNSCFG=\"%s\",\"%s\"\n", pri_dns, sec_dns);
	setCMD(txt);

}

bool cMdmCfgDNS::handleResponse(const char* response)
{
	char cmdReply[32];
	int cmdLen;

	//diag_printf("response: %s\n", response);

	strcpy(cmdReply,"+CDNSCFG:");
	cmdLen = strlen(cmdReply);
	if(!strncmp(cmdReply,response,cmdLen))
	{
		//diag_printf("=? response: %s\n", &response[cmdLen]);

	}

	strcpy(cmdReply, "PrimaryDns:");
	cmdLen = strlen(cmdReply);
	if(!strncmp(cmdReply,response,cmdLen))
	{
		//diag_printf("primary response: %s\n", &response[cmdLen]);
		strcpy(mPrimary, &response[cmdLen]);
	}

	strcpy(cmdReply, "SecondaryDns:");
	cmdLen = strlen(cmdReply);
	if(!strncmp(cmdReply,response,cmdLen))
	{
		//diag_printf("secondary response: %s\n", &response[cmdLen]);
		strcpy(mSecondary, &response[cmdLen]);
	}


	return false;
}

void cMdmCfgDNS::getDNS(char* pri_dns, char* sec_dns)
{
	if(pri_dns)
		strcpy(pri_dns, mPrimary);

	if(sec_dns)
		strcpy(sec_dns, mSecondary);
}

cMdmConnStat::cMdmConnStat()
	: cMdm_cmd("AT+CIPSTATUS\n")
{
}

cMdmStartUDP::cMdmStartUDP(char* ip, int port)
	: cMdm_cmd("AT+CIPSTART=\"UDP\",\"%s\",%d\n", ip, port)
{
}

cMdmSetPrompt::cMdmSetPrompt(prompt p) : cMdm_cmd("AT+CIPSPRT=%d\n", p)
{
}

cMdmSetRXHeader::cMdmSetRXHeader(ipHeader h) : cMdm_cmd("AT+CIPHEAD=%d\n", h)
{
}

cMdmSend::cMdmSend() : cMdm_cmd("")
{
	//diag_printf("SEND: new\n");
}

bool cMdmSend::handleResponse(const char* response)
{
	printf("SEND: response: %s\n", response);
	return true;
}

int cMdmSend::send(void* buff, cyg_uint16 len)
{
	int sentLen = 0;
	int cmdCnt = 0;
	char tempStr[32];

	sprintf(tempStr,"AT+CIPSEND=%d\n", len);
	//diag_printf("sending CMD: %d: %s \n", strlen(tempStr), tempStr);

	cModem::get()->write(tempStr);
	cyg_thread_delay(100);

	replied = false;
	len = cModem::get()->write(buff, len);
	cModem::get()->write("\n");

	//diag_printf("MDMD: Sent\n");
	//diag_dump_buf(buff, len);

	do
	{
		//Sleep(100);
		if(waitReply())
		{
			//diag_printf("RX data (%d) %s\n", mRxLen, (char*) mRxBuff);

			if(mRxLen > 0)
			{
				//diag_printf("RX data (%d) %s\n", mRxLen, (char*) mRxBuff);

				if(!strcmp((char*)mRxBuff, "SEND OK\r"))
				{
					sentLen = len;
					break;
				}
			}
			replied = false;
		}
		//diag_printf("MDM: Time out\n");
	}while(cmdCnt++ < 5);

	replied = false;
	return sentLen;
}

cMdmShut::cMdmShut() : cMdm_cmd("AT+CIPSHUT\n")
{
}

bool cMdmShut::handleResponse(const char* response)
{
	char cmdReply[6];
	int cmdLen;

	//printf("response: %s\n", response);

	strcpy(cmdReply,"SHUT OK");
	cmdLen = strlen(cmdReply);
	if(!strncmp(cmdReply,response,cmdLen))
	{
		return true;
	}

	return false;
}
cMdmGetLocalBaud::cMdmGetLocalBaud() : cMdm_cmd("AT+IPR?\n")
{
	mBaud = 0;
}

bool cMdmGetLocalBaud::handleResponse(const char* response)
{
	char cmdReply[16];
	int cmdLen;

	//printf("response: %s\n", response);

	strcpy(cmdReply,"+IPR:");
	cmdLen = strlen(cmdReply);
	if(!strncmp(cmdReply,response,cmdLen))
	{
		char *argv[2];
		int argc = 2;
		util_parse_params((char*)&response[cmdLen],argv,argc,'=',',');
		if(argc > 0)
		{
			mBaud = strtoul(argv[0],0,10);
		}
	}

	return false;
}

cMdmSetFixedBaud::cMdmSetFixedBaud(cyg_uint32 baud) : cMdm_cmd("AT+IPR=%d\n", baud)
{
}


