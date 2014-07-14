#include <stdio.h>

#include "nvm.h"
#include "definitions.h"
#include "spi_flash.h"
#include "crc.h"
#include "version.h"

cNVM * cNVM::__instance = 0;

void cNVM::init(cyg_uint32 dataAddress, cyg_uint32 statusAddress)
{
	if(__instance == 0)
	{
		__instance = new cNVM(dataAddress, statusAddress);
	}
}

cNVM * cNVM::get()
{
   return __instance;
}


cNVM::cNVM(cyg_uint32 dataAddress, cyg_uint32 statusAddress)
{
	mNVMDataAddress = dataAddress;
	mNVMStatusAddress = statusAddress;

	sNvmData temp_data;

	SpiFlash::get()->ReadSpi(mNVMDataAddress,(cyg_uint8 *)&temp_data,sizeof(sNvmData));
	if(check_crc(&temp_data))
	{
		diag_printf("NVM CRC OK\n");
		mNvmData = temp_data;
	}
	else
	{
		set_defaults();
		update();
	}

	sDeviceStat temp_stat;

	SpiFlash::get()->ReadSpi(mNVMStatusAddress,(cyg_uint8 *)&temp_stat,sizeof(temp_stat));
	if(check_crc(&temp_stat))
	{
		diag_printf("STAT CRC OK\n");
		mDevStat = temp_stat;
	}
	else
	{
		for (int k = 0; k < 4; ++k)
		{
			mDevStat.analogSampleRange[k] = 5;
			mDevStat.analogUpperLimit[k] = 500;
			mDevStat.analogLowerLimit[k] = -500;

		}

		mDevStat.logPeriod = 600;

		updateStat();
	}
}

cyg_uint32 cNVM::getVersion()
{
	return VERSION_NUM;
}

char* cNVM::getBuildDate()
{
	return BUILD_DATE;
}

cyg_uint32 cNVM::getSerial()
{
	if(!check_crc(&mNvmData))
	{
		sNvmData temp_data;

		SpiFlash::get()->ReadSpi(mNVMDataAddress, (cyg_uint8 *)&temp_data,sizeof(sNvmData));
		if(check_crc(&temp_data))
			mNvmData = temp_data;
	}

   return mNvmData.rmm_ser_num;
}

void cNVM::setSerial(cyg_uint32 s)
{
   mNvmData.rmm_ser_num = s;
   update();
}

void cNVM::setHWRev(char * hwr)
{
   strncpy((char *)mNvmData.hw_revision,(const char*)hwr,(cyg_uint8)NVM_STR_LEN);
   mNvmData.hw_revision[NVM_STR_LEN -1] = 0;
   update();
}

char * cNVM::getHWRev()
{
   return (char *)mNvmData.hw_revision;
}

float cNVM::getSampleRange(cyg_uint8 port)
{
	if(port < 4)
		return mDevStat.analogSampleRange[port];

	return 0xFF;
}

void cNVM::setSampleRange(cyg_uint8 port, float stat)
{
	if(port >= 4)
		return;

	if(mDevStat.analogSampleRange[port] != stat)
	{
		printf("Saving new diff on port:%d to %0.1f\n", port, stat);
		//diag_printf("DevStat was: %d, \n", mDevStat.defStatus[device] );
		mDevStat.analogSampleRange[port] = stat;
		updateStat();
	}
}

float cNVM::getUpperLimit(cyg_uint8 port)
{
	if(port < 4)
		return mDevStat.analogUpperLimit[port];

	return 0xFF;
}

void cNVM::setUpperLimit(cyg_uint8 port, float stat)
{
	if(port >= 4)
		return;

	if(mDevStat.analogUpperLimit[port] != stat)
	{
		printf("Saving new upper limit on port:%d to %0.1f\n", port, stat);
		//diag_printf("DevStat was: %d, \n", mDevStat.defStatus[device] );
		mDevStat.analogUpperLimit[port] = stat;
		updateStat();
	}
}

float cNVM::getLowerLimit(cyg_uint8 port)
{
	if(port < 4)
		return mDevStat.analogLowerLimit[port];

	return 0xFF;
}

void cNVM::setLowerLimit(cyg_uint8 port, float stat)
{
	if(port >= 4)
		return;

	if(mDevStat.analogLowerLimit[port] != stat)
	{
		printf("Saving new lower limit on port:%d to %0.1f\n", port, stat);
		//diag_printf("DevStat was: %d, \n", mDevStat.defStatus[device] );
		mDevStat.analogLowerLimit[port] = stat;
		updateStat();
	}
}

void cNVM::setLogPeriod(cyg_uint32 period)
{
	if(mDevStat.logPeriod != period)
	{
		diag_printf("Saving new log period to %ds\n", period);
		//diag_printf("DevStat was: %d, \n", mDevStat.defStatus[device] );
		mDevStat.logPeriod = period;
		updateStat();
	}
}

cyg_uint32 cNVM::getLogPeriod()
{
	return mDevStat.logPeriod;
}

cyg_uint64 cNVM::getBoxSerial()
{
	if(!check_crc(&mNvmData))
	{
		sNvmData temp_data;

		SpiFlash::get()->ReadSpi(mNVMDataAddress,(cyg_uint8 *)&temp_data,sizeof(sNvmData));
		if(check_crc(&temp_data))
			mNvmData = temp_data;
	}

	return mNvmData.box_ser_num;
}

void cNVM::setBoxSerial(cyg_uint64 ser)
{
	mNvmData.box_ser_num = ser;
	update();
}

cyg_uint32 cNVM::getUpdatePeriod()
{
	return mNvmData.updatePeriod;
}

void cNVM::setUpdatePeriod(cyg_uint32 ser)
{
	mNvmData.rmm_ser_num = ser;
	update();
}

void cNVM::setDefault()
{
	set_defaults();
	update();
}

cyg_bool cNVM::check_crc(sNvmData * d)
{
   cyg_uint16 calc_crc = cCrc::ccitt_crc16((cyg_uint8 *)d,sizeof(sNvmData)-2);
   if(calc_crc != d->crc)
   {
      diag_printf("NVM CRC Error 0x%04X != 0x%04X\n",calc_crc,d->crc);
      return false;
   }
   return true;
}

cyg_bool cNVM::check_crc(sDeviceStat * d)
{
   cyg_uint16 calc_crc = cCrc::ccitt_crc16((cyg_uint8 *)d,sizeof(sDeviceStat)-2);
   if(calc_crc != d->crc)
   {
      diag_printf("STAT CRC Error 0x%04X != 0x%04X\n",calc_crc,d->crc);
      return false;
   }
   return true;
}



void cNVM::set_defaults()
{
	dbg_printf(1,"\nSetting defaults\n");
   mNvmData.rmm_ser_num = 0x87654321;
   mNvmData.box_ser_num = 0;

   strcpy((char*)&mNvmData.hw_revision[0],"May-2012");

   set_connection_defaults();
}

void cNVM::updateStat()
{
	mDevStat.crc = cCrc::ccitt_crc16((cyg_uint8 *)&mDevStat,sizeof(sDeviceStat)-2);

	SpiFlash::get()->flash_erase(mNVMStatusAddress);
	SpiFlash::get()->WriteSpi(mNVMStatusAddress,(cyg_uint8 *)&mDevStat,sizeof(mDevStat));
}

void cNVM::update()
{
	dbg_printf(1,"\n\tWRITE\n");
   mNvmData.crc = cCrc::ccitt_crc16((cyg_uint8 *)&mNvmData,sizeof(sNvmData)-2);

   SpiFlash::get()->flash_erase(mNVMDataAddress);
   SpiFlash::get()->WriteSpi(mNVMDataAddress,(cyg_uint8 *)&mNvmData,sizeof(sNvmData));
}

void cNVM::setAPN(char *apn)
{
   strncpy((char *)mNvmData.apn,apn,NVM_STR_LEN);
   mNvmData.apn[NVM_STR_LEN-1] = 0;
   update();
}

char* cNVM::getAPN()
{
   return (char *)mNvmData.apn;
}

void cNVM::setUser(char *user)
{
   strncpy((char *)mNvmData.apn_user,user,NVM_STR_LEN);
   mNvmData.apn_user[NVM_STR_LEN-1] = 0;
   update();
}

char* cNVM::getUser()
{
   return (char *)mNvmData.apn_user;
}

void cNVM::setPasswd(char *pwd)
{
   strncpy((char *)mNvmData.apn_pass,pwd,NVM_STR_LEN);
   mNvmData.apn_pass[NVM_STR_LEN-1] = 0;
   update();
}

char* cNVM::getPasswd()
{
   return (char *)mNvmData.apn_pass;
}

void cNVM::setServer(char * server)
{
   strncpy((char *)mNvmData.server_name,server,NVM_SERVER_NAME_LEN);
   mNvmData.server_name[NVM_SERVER_NAME_LEN -1] = 0;
   update();
}

char* cNVM::getServer()
{
   return (char *)mNvmData.server_name;
}

void cNVM::setPort(cyg_uint16 port)
{
   mNvmData.server_port = port;
   update();
}
cyg_uint16 cNVM::getPort()
{
   return mNvmData.server_port;
}

void cNVM::setSimCell(char * cell)
{
   strncpy((char *)mNvmData.sim_cell,cell,NVM_SIM_CELL_LEN);
   mNvmData.sim_cell[NVM_SIM_CELL_LEN - 1] = 0;
   update();
}

char* cNVM::getSimCell()
{
   return (char *)mNvmData.sim_cell;
}

void cNVM::setSimPin(char * pin)
{
   strncpy((char *)mNvmData.sim_pin,pin,NVM_SIM_PIN_LEN);
   mNvmData.sim_pin[NVM_SIM_PIN_LEN-1] = 0;
   update();
}

char* cNVM::getSimPin()
{
   return (char *)mNvmData.sim_pin;
}

void cNVM::setSimPuk(char * puk)
{
   strncpy((char *)mNvmData.sim_puk,puk,NVM_SIM_PUK_LEN);
   mNvmData.sim_puk[NVM_SIM_PUK_LEN - 1] = 0;
   update();
}

char* cNVM::getSimPuk()
{
   return (char *)mNvmData.sim_puk;
}

void cNVM::setSimPukFlag(bool stat)
{
	if(mNvmData.sim_puk_flag != stat)
	{
		mNvmData.sim_puk_flag = stat;
		update();
	}
}

bool cNVM::getSimPukFlag()
{
   return mNvmData.sim_puk_flag;
}

void cNVM::set_connection_defaults()
{
	mNvmData.updatePeriod = 86400;
	strcpy((char*)&mNvmData.apn[0],"internet");
	strcpy((char*)&mNvmData.apn_pass[0],"");
	strcpy((char*)&mNvmData.apn_user[0],"");
	strcpy((char*)&mNvmData.server_name[0],"www.ostb.co.za");
	mNvmData.server_port = 7766;

   strcpy((char*)&mNvmData.sim_cell[0],"00000000");
   strcpy((char*)&mNvmData.sim_pin[0],"0000");
   strcpy((char*)&mNvmData.sim_puk[0],"00000000");
   mNvmData.sim_puk_flag = false;

}

cNVM::~cNVM()
{
}

cNVM::sNvmData::sNvmData()
{
	rmm_ser_num = 0xFFFFFFFF;
	box_ser_num = 0;
	crc = 0;
	server_name[0] = 0;
	server_port = 0;
	sim_puk_flag = 0;
	updatePeriod = 0;
}
