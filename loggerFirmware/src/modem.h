#ifndef MODEM_H_
#define MODEM_H_
#include <cyg/kernel/kapi.h>
#include <cyg/io/io.h>

#include "definitions.h"
#include "debug.h"
#include "mdm_cmd.h"

#define MODEM_BUFF_SIZE 1024

class cModem : public cDebug
{
public:
	enum eModemStat
		{
			Off,
			needSIM,
			needPIN,
			needPUK,
			NetworkBusy,
			NetworkReg,
			GPRSatt,
			Linked
		}mStat;

private:
	static cModem* _instance;

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

    cMdm_cmd* mCurrCMD;
	cModem(char* serDev);

	enum eSIMstat
	{
		SIMneeded,
		SIMpin,
		SIMpuk,
		SIMready
	}mSIM;

	enum eRegStat
	{
		notRegistered = 0,
		registered,
		searching,
		denied,
		unknown,
		roaming
	}mReg;

	enum eConnStat
	{
		initial,
		connecting,
		connected,
		gprsActive,
		closing,
		closed
	}mConnection;

	void handleURC(const char* response);

    bool showID();
    bool isSIMinserted();
    cMdmPINstat::ePINstat getPINstat();
    int getSQuality();
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

    void receive();


public:
	static void init(char* serDev);
	static cModem* get();

	void power(bool stat);
	void reset();


	void doCMD();

	bool setEcho(bool stat);
	eModemStat getStatus();
	cModem::eSIMstat simStatus();
    bool insertPIN(char* pin);
    bool insertPUK(char* puk, char* pin);
    bool upModemLink(char* address, int port, char *apn, char* user_name, char* password);
	void showStatus(eModemStat stat);
	bool IPstatus();
	bool shutIP();
	bool setFixedBaud();

	cyg_uint16 write(const char* str);
	cyg_uint16 write(void* buff, cyg_uint16 len);

	cyg_uint16 send(void* buff, cyg_uint16 len);

	virtual ~cModem();
};





#endif /* MODEM_H_ */
