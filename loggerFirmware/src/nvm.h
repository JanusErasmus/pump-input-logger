#ifndef _NVM_H_
#define _NVM_H_
#include <cyg/kernel/kapi.h>

#include "debug.h"
#include "term.h"

#define NVM_SECTOR 0x00
#define NVM_MON_CNT 10
#define NVM_STR_LEN 24
#define NVM_SERVER_NAME_LEN 64
#define NVM_SIM_PIN_LEN 6
#define NVM_SIM_PUK_LEN 10
#define NVM_SIM_CELL_LEN 16
#define NVM_STAT_SECTOR 0x10000


class cNVM : public cDebug
{
private:

	static cNVM * __instance;
	cNVM();
	virtual ~cNVM();

	bool mReadyFlag;

	struct sNvmData
	{
		cyg_uint32 rmm_ser_num;
		cyg_uint64 box_ser_num;
		cyg_uint8 hw_revision[NVM_STR_LEN];

		cyg_uint32 updatePeriod;
		cyg_uint8 apn[NVM_STR_LEN];		///APN name for GPRS access
		cyg_uint8 apn_user[NVM_STR_LEN];	///The APN user name. Mostly this is an empty string
		cyg_uint8 apn_pass[NVM_STR_LEN];	///APN password. Mostly empty
		cyg_uint8 server_name[NVM_SERVER_NAME_LEN];///The address of the RMM server we connect to.
		cyg_uint16 server_port;			//The UDP port we use to connect to the GLAM server

		cyg_uint8 sim_cell[NVM_SIM_CELL_LEN];
		cyg_uint16 sim_pin[NVM_SIM_PIN_LEN];
		cyg_uint8 sim_puk[NVM_SIM_PUK_LEN];
		cyg_bool sim_puk_flag;

		cyg_uint16 crc;


		sNvmData();

	} __attribute__((__packed__)) mNvmData;

	struct sDeviceStat
	{
		cyg_uint8 outputDefaultStatus[NVM_MON_CNT];
		cyg_uint8 inputDefaultStatus[NVM_MON_CNT];
		cyg_uint8 analogDefaultSamplerate[NVM_MON_CNT];
		float sampleRange[NVM_MON_CNT];

		cyg_uint8 pumpFrameStart;
		cyg_uint8 pumpFrameEnd;

		cyg_uint8 pumpUpTime;
		cyg_uint8 pumpRestTime;

		cyg_uint16 crc;
	} __attribute__((__packed__)) mDevStat;


	void update();
	void updateStat();

	cyg_bool check_crc(sNvmData * d);
	cyg_bool check_crc(sDeviceStat * d);

	void set_defaults();
	void setDefault(void);
	void set_connection_defaults();

	cyg_bool readNVM(sNvmData* temp_data);

public:

	static cNVM * get();
	bool isReady();

	cyg_uint32 getSerial();
	void setSerial(cyg_uint32);
	cyg_uint64 getBoxSerial();
	void setBoxSerial(cyg_uint64);
	void setHWRev(char*);
	char * getHWRev();
	cyg_uint32 getUpdatePeriod();
	void setUpdatePeriod(cyg_uint32);
	char* getAPN();
	void setAPN(char* apn);
	char* getUser();
	void setUser(char* user);
	char* getPasswd();
	void setPasswd(char* pwd);
	char* getServer();
	void setServer(char* server);
	cyg_uint16 getPort();
	void setPort(cyg_uint16 port);
	void setSimCell(char* cell);
	char* getSimCell();
	void setSimPin(char* pin);
	char* getSimPin();
	void setSimPuk(char* puk);
	char* getSimPuk();
	void setSimPukFlag(bool stat);
	bool getSimPukFlag();



	void setOutputStat(cyg_uint8,cyg_uint8);
	cyg_uint8 getOutputStat(cyg_uint8);

	void setInputStat(cyg_uint8,cyg_uint8);
	bool getInputStat(cyg_uint8);

	void setAnalogStat(cyg_uint8,cyg_uint8);
	cyg_uint8 getAnalogStat(cyg_uint8);

	void setSampleRange(cyg_uint8 port, float rate);
	float getSampleRange(cyg_uint8 port);

	void setPumpFrameStart(cyg_uint8);
	cyg_uint8 getPumpFrameStart();
	void setPumpFrameEnd(cyg_uint8);
	cyg_uint8 getPumpFrameEnd();

	void setPumpUpTime(cyg_uint8);
	cyg_uint8 getPumpUpTime();
	void setPumpRestTime(cyg_uint8);
	cyg_uint8 getPumpRestTime();

	static void config(cTerm & t,int mArgc,char *mArgv[]);
	static void nvmBuff(cTerm & t,int argc,char *argv[]);

};

#endif //Include Guard
