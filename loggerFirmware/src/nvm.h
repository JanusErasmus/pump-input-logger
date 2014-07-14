#ifndef _NVM_H_
#define _NVM_H_
#include <cyg/kernel/kapi.h>

#include "debug.h"

#define NVM_STR_LEN 24
#define NVM_SERVER_NAME_LEN 64
#define NVM_SIM_PIN_LEN 6
#define NVM_SIM_PUK_LEN 10
#define NVM_SIM_CELL_LEN 16

class cNVM : public cDebug
{
private:

   static cNVM * __instance;
   cNVM(cyg_uint32 dataAddress, cyg_uint32 statusAddress);
   virtual ~cNVM();

   cyg_uint32 mNVMDataAddress;
   cyg_uint32 mNVMStatusAddress;

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
	   float analogSampleRange[4];
	   float analogUpperLimit[4];
	   float analogLowerLimit[4];
	   cyg_uint32 logPeriod;

	   cyg_uint16 crc;
   } __attribute__((__packed__)) mDevStat;


   void update();
   void updateStat();

   cyg_bool check_crc(sNvmData * d);
   cyg_bool check_crc(sDeviceStat * d);

   void set_defaults();
   void set_connection_defaults();

public:

   static void init(cyg_uint32 dataAddress, cyg_uint32 statusAddress);
   static cNVM *get();

   cyg_uint32 getVersion();
   char* getBuildDate();

   void setSampleRange(cyg_uint8,float);
   float getSampleRange(cyg_uint8);
   void setUpperLimit(cyg_uint8,float);
   float getUpperLimit(cyg_uint8);
   void setLowerLimit(cyg_uint8,float);
   float getLowerLimit(cyg_uint8);
   void setLogPeriod(cyg_uint32);
   cyg_uint32 getLogPeriod();

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

   void setDefault(void);
};

#endif //Include Guard
