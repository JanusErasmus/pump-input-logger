#ifndef CMDMCMD_H_
#define CMDMCMD_H_
#include <cyg/kernel/kapi.h>

class cMdm_cmd
{
	cyg_sem_t mWaitSema;

	cyg_mutex_t mBuffMutex;

	char* mCMD;
	char mTempStr[128];

protected:

	enum eMdm_response
		{
			ok,
			err,
			unknown
		};

	cyg_mutex_t mDataHandleMutex;
	cyg_cond_t mDataHandleCond;

	cyg_uint8* mRxBuff;//[512];
	cyg_uint8 mRxLen;

	cMdm_cmd(const char * fmt,...);

	cyg_uint8* getCMD();

	eMdm_response getResponse(const char* response);
	virtual bool handleResponse(const char* response);

	bool parseValue(const char* buff, char delimiter, int* paramC, char paramV[][32]);

	void setCMD(char*);
	bool waitReply();

public:
	virtual  ~cMdm_cmd();

	bool execute();
	void handleData(cyg_uint8* data, int len);
	int waitData(char* buff);

};

class cMdmGetID : public cMdm_cmd
{
	char id[16];

	bool handleResponse(const char* response);
public:
	cMdmGetID();

	char* getID(){ return id; };
};

class cMdmSetEcho : public cMdm_cmd
{
public:
	cMdmSetEcho(bool stat);
};

class cMdmGetPBCount : public cMdm_cmd
{
	int mCount;
	int mSize;

	bool handleResponse(const char* response);
public:
	cMdmGetPBCount();

	int count(){ return mCount; };
	int size(){ return mSize; };

};

class cMdmGetPB : public cMdm_cmd
{
public:
	struct s_entry
	{
		char number[40];
		char name[16];
	};

private:
	s_entry* mList;
	int mCurrIdx;
	int mCount;

	bool handleResponse(const char* response);
public:
	cMdmGetPB(s_entry* list, int count);

};

class cMdmUpdatePB : public cMdm_cmd
{

public:
	cMdmUpdatePB(int idx, const char* name, const char* number);
	cMdmUpdatePB(int idx);

};

class cMdmSIMinserted : public cMdm_cmd
{
	bool mInserted;

	bool handleResponse(const char* response);
public:
	cMdmSIMinserted();

	bool inserted(){ return mInserted; };


};

class cMdmPINstat : public cMdm_cmd
{
public:
	enum ePINstat
	{
		UNKNOWN,
		READY,
		SIM_PIN,
		SIM_PUK,
		PH_SIM_PIN,
		PH_SIM_PUK,
		SIM_PIN2,
		SIM_PUK2
	};

private:
	ePINstat mStat;

	bool handleResponse(const char* response);
public:
	cMdmPINstat();

	ePINstat stat(){ return mStat; };

};

class cMdmEnterPIN : public cMdm_cmd
{
public:
	cMdmEnterPIN(char* pin);
	cMdmEnterPIN(char* puk, char* pin);

};

class cMdmSignalQ : public cMdm_cmd
{
	int mRSSI;

	bool handleResponse(const char* response);
public:
	cMdmSignalQ();

	int strength(){ return mRSSI; };


};

class cMdmNetOperator : public cMdm_cmd
{
	char mOperator[32];

	bool handleResponse(const char* response);
public:
	cMdmNetOperator();

	char* name(){ return mOperator; };


};

class cMdmCallReady : public cMdm_cmd
{
	bool mStat;

	bool handleResponse(const char* response);
public:
	cMdmCallReady();

	bool stat(){ return mStat; };
};

class cMdmNetRegistration : public cMdm_cmd
{
	bool mStat;

	bool handleResponse(const char* response);
public:
	cMdmNetRegistration();

	bool stat(){ return mStat; };
};

class cMdmPlaceCall : public cMdm_cmd
{
public:
	cMdmPlaceCall(const char* number);
	cMdmPlaceCall(int pbIndex);

};


class cMdmEndCall : public cMdm_cmd
{
public:
	cMdmEndCall();
};

class cMdmSendSMS : public cMdm_cmd
{
public:
	cMdmSendSMS();
	bool send(const char* number, const char* text);
};

class cMdmReadSMS : public cMdm_cmd
{
public:
	struct sSMS
	{
		cyg_uint8 mIdx;
		char mNumber[13];
		char mName[16];
		char mTime[22];
		char mText[160];

		sSMS(cyg_uint8 idx, const char* number, const char* name , const char* time);
		void setText(const char* text);
		void show();

		~sSMS(){};
	};

private:
	sSMS ** mSMSlist;
	int listIdx;
	bool smsString;

	bool handleResponse(const char* response);

public:
	cMdmReadSMS(sSMS ** list);
};

class cMdmDeleteSMS : public cMdm_cmd
{
public:
	cMdmDeleteSMS();
};

class cMdmUSSD : public cMdm_cmd
{
public:
	cMdmUSSD(const char* ussd);
};

class cMdmUSSDoff : public cMdm_cmd
{
public:
	cMdmUSSDoff();
};


class cMdmGPRSattched : public cMdm_cmd
{
	bool mStat;

	bool handleResponse(const char* response);
public:
	cMdmGPRSattched();

	bool stat(){ return mStat; };


};

class cMdmSetGSMalphabet : public cMdm_cmd
{
public:
	cMdmSetGSMalphabet();


};

class cMdmSetIPstate : public cMdm_cmd
{
public:
	enum IPconn
	{
		UNKNOWN = -1,
		SINGLE = 0,
		MULTI = 1,
	};

private:

	IPconn mIPstate;

	bool handleResponse(const char* response);
public:
	cMdmSetIPstate();

	void state(IPconn);

	IPconn IPconnState(){ return mIPstate; };

};

class cMdmSetIPmode : public cMdm_cmd
{
public:
	enum IPmode
	{
		UNKNOWN = -1,
		NORMAL = 0,
		TRANS = 1
	};

private:

	IPmode mIPmode;

	bool handleResponse(const char* response);
public:
	cMdmSetIPmode();

	void mode(IPmode);

	IPmode IPconnMode(){ return mIPmode; };

};

class cMdmSetGPRSconnection : public cMdm_cmd
{
public:
	cMdmSetGPRSconnection(char* apn, char* user_name, char* password);
};

class cMdmSetLocalPort : public cMdm_cmd
{
public:
	enum Lmode
	{
		UNKNOWN = -1,
		TCP = 0,
		UDP = 1
	};

private:

	Lmode mPmode;
	cyg_uint16 mTCPport, mUDPport;


	bool handleResponse(const char* response);
public:
	cMdmSetLocalPort();

	void setPort(Lmode, cyg_uint16);

	cyg_uint16 getPort(Lmode);

};


class cMdmSetRXIPshow : public cMdm_cmd
{
public:
	cMdmSetRXIPshow( bool stat);
};


class cMdmCfgTransparentTx : public cMdm_cmd
{
public:
	cMdmCfgTransparentTx(int retryN, int waitN, int buffLen, bool escSeq);
};

class cMdmStartTask : public cMdm_cmd
{
public:
	cMdmStartTask();
};


class cMdmUpConnection : public cMdm_cmd
{
public:
	cMdmUpConnection();
};

class cMdmGetLocalIP : public cMdm_cmd
{
	char mIP[32];
	bool isIP(const char*);

	bool handleResponse(const char* response);
public:
	cMdmGetLocalIP();
	void setIP(char*);
};


class cMdmCfgDNS : public cMdm_cmd
{
	char mPrimary[32];
	char mSecondary[32];

	bool handleResponse(const char* response);
public:
	cMdmCfgDNS();
	void setDNS(char*, char*);
	void getDNS(char*, char*);
};

class cMdmConnStat : public cMdm_cmd
{
public:
	cMdmConnStat();

};

class cMdmStartUDP : public cMdm_cmd
{
public:
	cMdmStartUDP(char* ip, int port);
};

class cMdmSetRXHeader : public cMdm_cmd
{
public:
	enum ipHeader
	{
		noHeader = 0,
		header = 1,

	};

	cMdmSetRXHeader(ipHeader);
};

class cMdmSetPrompt : public cMdm_cmd
{
public:
	enum prompt
	{
		noPrompt = 0,
		promptOK = 1,
		OKonly = 2
	};

	cMdmSetPrompt(prompt);
};

class cMdmSend : public cMdm_cmd
{
	char* mBuff;
	int mLen;
	bool handleResponse(const char* response);
public:
	cMdmSend();

	int send(void* buff, cyg_uint16 len);
};

class cMdmShut : public cMdm_cmd
{
	bool handleResponse(const char* response);
public:
	cMdmShut();
};

class cMdmGetLocalBaud : public cMdm_cmd
{
	cyg_uint32 mBaud;

	bool handleResponse(const char* response);
public:
	cMdmGetLocalBaud();
	cyg_uint32 getBaud(){ return mBaud; };
};

class cMdmSetFixedBaud : public cMdm_cmd
{


public:
	cMdmSetFixedBaud(cyg_uint32 baud);
};

#endif /* CMDMCMD_H_ */
