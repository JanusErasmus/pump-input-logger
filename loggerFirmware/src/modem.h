#ifndef MODEM_H_
#define MODEM_H_
#include <cyg/kernel/kapi.h>
#include <cyg/io/io.h>

#include "definitions.h"
#include "term.h"
#include "debug.h"
#include "mdm_cmd.h"

#define MODEM_BUFF_SIZE 1024

class cAlarmCallback
{
public:
	virtual void acknowledgeAlarm() = 0;

	virtual ~cAlarmCallback(){};
};

class cModem : public cDebug
{
public:
	enum eModemStat
		{
			Off,
			SIMnotReady,
			NetworkSearching,
			NetworkBusy,
			NetworkCallReady,
			NetworkRegistered,
			GPRSatt,
			Linked
		};

	enum eSIMstat
		{
			SIMneeded,
			SIMpin,
			SIMpuk,
			SIMready
		};

private:
	static cModem* _instance;

	eModemStat mModemStatus;
	cyg_mutex_t mModemStatusMutex;

    cyg_io_handle_t mSerCMDHandle;
    cyg_io_handle_t mSerDataHandle;

    cyg_uint8 mRXbuff[MODEM_BUFF_SIZE];
    cyg_uint32 mRXlen;
    cyg_mutex_t mRXmutex;

    cyg_uint8 mStack[MDM_STACK_SIZE];
    cyg_thread mThread;
    cyg_handle_t mThreadHandle;
    static void rx_thread_func(cyg_addrword_t arg);
    void run();

    float mBalance;
    cyg_mutex_t mBalanceMutex;
    cyg_cond_t mBalanceCond;

    cyg_mutex_t mCurrCMDMutex;
    cMdm_cmd* mCurrCMD;
	cModem(char* serDev);

	eSIMstat mSIMstatus;

	enum eConnStat
	{
		IPunknown,
		IPinitial,
		IPconnecting,
		IPconnected,
		IPgprsActive,
		IPclosing,
		IPclosed
	}mConnection;

	cModem::eModemStat retrieveModemStatus();
	eSIMstat simStatus();

	cyg_mutex_t mCallBusyMutex;
	cyg_cond_t mCallBusyCond;
	void handleURC(const char* response);
	cAlarmCallback * mAckAlarm;

    bool showID();
    bool isSIMinserted();
    cMdmPINstat::ePINstat getPINstat();
    int getSQuality();
    bool isCallReady();
    bool isRegistered();
    bool getNetOperator(char* netOperator);
    bool isGPRSattatched();
    bool setGSMalphabet();
    bool setIPstate(cMdmSetIPstate::IPconn conStat);
    bool setIPmode(cMdmSetIPmode::IPmode mode);
    bool setGPRSconnection(char *apn, char* user_name, char* password);
    bool setPort(cMdmSetLocalPort::Lmode mode, cyg_uint16 port);
    bool setRXIPshow(bool shown);
    bool setTransparentCFG(int retryN, int waitN, int buffLen, bool escSeq);
    bool upConnection();
    bool getLocalIP(char *ip);
    bool getDNS(char *priDNS, char *secDNS);
    bool disablePrompt();
    bool link(char *address, int port);

    void listPhoneBook();
    void updatePhoneBook(int idx, const char* name, const char* number);
    void readSMS();

    void receive();


public:
	static void init(char* serDev);
	static cModem* get();

	void power(bool stat);
	void reset();

	void updateStatus();
	void showModemStatus(eModemStat stat);
	void showSIMStatus(eSIMstat stat);

	void doCMD();

	bool setEcho(bool stat);

	eModemStat getModemStatus();
	eSIMstat getSIMstatus();

    bool insertPIN(char* pin);
    bool insertPUK(char* puk, char* pin);
    bool upModemLink(char* address, int port, char *apn, char* user_name, char* password);


	bool IPstatus();
	bool shutIP();
	bool setFixedBaud();

	void setAcknowledge(cAlarmCallback * ack){ mAckAlarm = ack; };
	bool missedCall(const char* number);
	bool missedCall(int number);
	bool sendSMS(const char* number, const char* text);
	bool getPhoneBook(cMdmGetPB::s_entry* list, int* count, int* size);
	bool getSMSlist(cMdmReadSMS::sSMS ** list);
	bool deleteSMS();

	void checkBalance();

	cyg_uint16 write(const char* str);
	cyg_uint16 write(void* buff, cyg_uint16 len);

	cyg_uint16 send(void* buff, cyg_uint16 len);


	static void debug(cTerm & term, int argc,char * argv[]);
	static void ATcmd(cTerm & term, int argc,char * argv[]);

	virtual ~cModem();
};





#endif /* MODEM_H_ */
