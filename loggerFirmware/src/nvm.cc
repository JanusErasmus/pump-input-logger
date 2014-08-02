#include <stdio.h>
#include <cyg/io/flash.h>
#include <stdlib.h>

#include "spi_dev.h"
#include "nvm.h"
#include "crc.h"
#include "led.h"

cNVM * cNVM::__instance = NULL;


cNVM * cNVM::get()
{
   if(__instance == NULL)
   {
      __instance = new cNVM();
   }
   return __instance;
}

cNVM::cNVM() : mReadyFlag(false)
{

	cyg_flash_info_t info;
	cyg_flash_init(0);
	int ret = cyg_flash_get_info(0,&info);
	if(!ret)
	{
		diag_printf("SPI_FLASH: Device:\n");
		diag_printf(" - Block:\n");
		diag_printf("   - info's: %d\n",info.num_block_infos);
		diag_printf("   - count : %d\n",info.block_info[0].blocks);
		diag_printf("   - size  : 0x%8.0X\n",info.block_info[0].block_size);
		diag_printf(" - START 0x%8.0X \n",info.start);
		diag_printf(" - END   0x%8.0X \n",info.end);
	}
	else
	{

		diag_printf("cyg_flash_info return value = %d\n", ret);
		diag_printf("Could not initialize SPI flash\n");
	}

	spi_dev_global_unprotect();

	sNvmData temp_data;
	if(readNVM(&temp_data))
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
	cyg_flash_read(NVM_STAT_SECTOR,(cyg_uint8 *)&temp_stat,sizeof(temp_stat), 0);
	if(check_crc(&temp_stat))
	{
		mDevStat = temp_stat;
	}
	else
	{
		for (int k = 0; k < NVM_MON_CNT; ++k)
		{
			mDevStat.outputDefaultStatus[k] = 0;
			mDevStat.inputDefaultStatus[k] = 0;
			mDevStat.analogDefaultSamplerate[k] = 0;
			mDevStat.sampleRange[k] = 1;

			mDevStat.pumpFrameStart = 0;
			mDevStat.pumpFrameEnd = 0;

			mDevStat.pumpUpTime = 1;
			mDevStat.pumpRestTime = 1;
		}

		updateStat();
	}

	mReadyFlag = true;
}

bool cNVM::isReady()
{
	return mReadyFlag;
}

cyg_uint32 cNVM::getSerial()
{
	//double check the CRC of the NVM data before returning serial
	if(!check_crc(&mNvmData))
	{
		sNvmData temp_data;
		if(readNVM(&temp_data))
		{
			mNvmData = temp_data;
		}
		else
		{
			set_defaults();
			update();
		}
	}

	return mNvmData.rmm_ser_num;
}

void cNVM::setSerial(cyg_uint32 s)
{
   mNvmData.rmm_ser_num = s;
   update();
}

cyg_uint8 cNVM::getOutputStat(cyg_uint8 device)
{
	return mDevStat.outputDefaultStatus[device];
}

void cNVM::setOutputStat(cyg_uint8 port, cyg_uint8 stat)
{
	if(mDevStat.outputDefaultStatus[port] != stat)
	{
		diag_printf("Saving default on output %d: %d\n", port, stat);
		//diag_printf("DevStat was: %d, \n", mDevStat.defStatus[device] );
		mDevStat.outputDefaultStatus[port] = stat;
		updateStat();
	}
}

bool cNVM::getInputStat(cyg_uint8 num)
{
	return mDevStat.inputDefaultStatus[num];
}

void cNVM::setInputStat(cyg_uint8 port, cyg_uint8 stat)
{
	//diag_printf("Dev %d, stat %d : set %d\n",device, mDevStat.inputStatus[device],stat);
	if(mDevStat.inputDefaultStatus[port] != stat)
	{
		diag_printf("Saving default on input %d: %d\n", port, stat);
		//diag_printf("DevStat was: %d, \n", mDevStat.inputStatus[device] );
		mDevStat.inputDefaultStatus[port] = stat;
		updateStat();
	}
}

cyg_uint8 cNVM::getAnalogStat(cyg_uint8 num)
{
	return mDevStat.analogDefaultSamplerate[num];
}

void cNVM::setAnalogStat(cyg_uint8 port, cyg_uint8 stat)
{
	//diag_printf("Dev %d, stat %d : set %d\n",device, mDevStat.inputStatus[device],stat);
	if(mDevStat.analogDefaultSamplerate[port] != stat)
	{
		diag_printf("Saving default on analog %d: %d\n", port, stat);
		//diag_printf("DevStat was: %d, \n", mDevStat.inputStatus[device] );
		mDevStat.analogDefaultSamplerate[port] = stat;
		updateStat();
	}
}

void cNVM::setSampleRange(cyg_uint8 port, float rate)
{
	if(mDevStat.sampleRange[port] != rate)
		{
			printf("Saving default on analog %d: %f\n", port, rate);
			//diag_printf("DevStat was: %d, \n", mDevStat.inputStatus[device] );
			mDevStat.sampleRange[port] = rate;
			updateStat();
		}
}

float cNVM::getSampleRange(cyg_uint8 port)
{
	return mDevStat.sampleRange[port];
}

cyg_bool cNVM::readNVM(sNvmData* temp_data)
{
	cyg_flash_read(NVM_SECTOR, (cyg_uint8 *) temp_data, sizeof(sNvmData), 0);

	if (check_crc(temp_data))
	{
		return true;
	}

	return false;
}

void cNVM::setPumpFrameStart(cyg_uint8 start)
{
	if(mDevStat.pumpFrameStart != start)
	{
		diag_printf("Saving frame start H: %d\n", start);
		mDevStat.pumpFrameStart = start;
		updateStat();
	}
}

cyg_uint8 cNVM::getPumpFrameStart()
{
	return mDevStat.pumpFrameStart ;
}

void cNVM::setPumpFrameEnd(cyg_uint8 end)
{
	if(mDevStat.pumpFrameEnd != end)
	{
		diag_printf("Saving frame start H: %d\n", end);
		mDevStat.pumpFrameEnd = end;
		updateStat();
	}
}

cyg_uint8 cNVM::getPumpFrameEnd()
{
	return mDevStat.pumpFrameEnd ;
}

void cNVM::setPumpUpTime(cyg_uint8 start)
{
	if(mDevStat.pumpUpTime != start)
	{
		diag_printf("Saving up Time: %d\n", start);
		mDevStat.pumpUpTime = start;
		updateStat();
	}
}

cyg_uint8 cNVM::getPumpUpTime()
{
	return mDevStat.pumpUpTime ;
}

void cNVM::setPumpRestTime(cyg_uint8 end)
{
	if(mDevStat.pumpRestTime != end)
	{
		diag_printf("Saving rest Time: %d\n", end);
		mDevStat.pumpRestTime = end;
		updateStat();
	}
}

cyg_uint8 cNVM::getPumpRestTime()
{
	return mDevStat.pumpRestTime ;
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
   diag_printf("STAT CRC OK\n");
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

	cyg_flash_erase(NVM_STAT_SECTOR, 1, 0);
	cyg_flash_program(NVM_STAT_SECTOR,(cyg_uint8 *)&mDevStat,sizeof(mDevStat), 0);
}

void cNVM::update()
{
	dbg_printf(1,"\n\tWRITE\n");
   mNvmData.crc = cCrc::ccitt_crc16((cyg_uint8 *)&mNvmData,sizeof(sNvmData)-2);

   cyg_flash_erase(NVM_SECTOR, 1, 0);
   cyg_flash_program(NVM_SECTOR,(cyg_uint8 *)&mNvmData,sizeof(sNvmData), 0);
}

cNVM::sNvmData::sNvmData()
{
	rmm_ser_num = 0x7FFFFFFF;
	box_ser_num = 0;
	crc = 0;
	server_name[0] = 0;
	server_port = 0;
	sim_puk_flag = 0;
	updatePeriod = 0;
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


void cNVM::nvmBuff(cTerm & t,int argc,char *argv[])
{
	if(!__instance)
		return;

	t<<(YELLOW("Default status:\n"));
	printf("  %-10s%-10s%-10s%-10s\n", "Output", "Input", "Analog", "Range");
	for(int k = 0; k < NVM_MON_CNT; k++)
	{
		printf("%d: %02X        %02X        %02d        %f\n", k, __instance->getOutputStat(k), __instance->getInputStat(k), __instance->getAnalogStat(k), __instance->getSampleRange(k));
	}

	t<<(YELLOW("\nPump Frame:\n"));
	printf(" - Start %02dH00\n", __instance->getPumpFrameStart());
	printf(" - End   %02dH00\n", __instance->getPumpFrameEnd());
	t<<(YELLOW("Pump Up/Rest Time:\n"));
	printf(" - UP   %02dh\n", __instance->getPumpUpTime());
	printf(" - Rest %02dh\n", __instance->getPumpRestTime());
}

void cNVM::config(cTerm & t,int mArgc,char *mArgv[])
{
	if(!__instance)
		return;

	t<<(YELLOW("Server configuration:\n"));

	int arg_idx = 1;
	while(arg_idx < mArgc)
	{
		if(!strcmp("apn",mArgv[arg_idx]))
		{
			// Move onto the parameter//
			arg_idx++;
			if(arg_idx < mArgc)
			{
				// We have a parameter //
				t<<"Setting APN to "<<mArgv[arg_idx]<<"\n";
				__instance->setAPN(mArgv[arg_idx]);
			}
			else
			{
				// Just print the APN name //
				t<<"APN="<<__instance->getAPN()<<"\n";
				return;
			}
		}
		else if(!strcmp("user",mArgv[arg_idx]))
		{
			// Move onto the parameter//
			arg_idx++;
			if(arg_idx < mArgc)
			{
				// We have a parameter //
				t<<"Setting User to "<<mArgv[arg_idx]<<"\n";
				__instance->setUser(mArgv[arg_idx]);
			}
			else
			{
				// Just print the User name //
				t<<"USER="<<__instance->getUser()<<"\n";
				return;
			}
		}
		else if(!strcmp("passwd",mArgv[arg_idx]))
		{
			// Move onto the parameter//
			arg_idx++;
			if(arg_idx < mArgc)
			{
				// We have a parameter //
				t<<"Setting Password to "<<mArgv[arg_idx]<<"\n";
				__instance->setPasswd(mArgv[arg_idx]);
			}
			else
			{
				// Just print the password name //
				t<<"PASSWD="<<__instance->getPasswd()<<"\n";
				return;
			}
		}
		else if(!strcmp("server",mArgv[arg_idx]))
		{
			// Move onto the parameter//
			arg_idx++;
			if(arg_idx < mArgc)
			{
				// We have a parameter //
				t<<"Setting server to "<<mArgv[arg_idx]<<"\n";
				__instance->setServer(mArgv[arg_idx]);
			}
			else
			{
				// Just print the Server name //
				t<<"SERVER="<<__instance->getServer()<<"\n";
				return;
			}
		}
		else if(!strcmp("cell",mArgv[arg_idx]))
				{
					// Move onto the parameter//
					arg_idx++;
					if(arg_idx < mArgc)
					{
						// We have a parameter //
						t<<"Setting SIM cell to "<<mArgv[arg_idx]<<"\n";
						__instance->setSimCell(mArgv[arg_idx]);
					}
					else
					{
						// Just print the Server name //
						t<<"SIM CELL="<<__instance->getSimCell()<<"\n";
						return;
					}
				}
		else if(!strcmp("pin",mArgv[arg_idx]))
		{
			// Move onto the parameter//
			arg_idx++;
			if(arg_idx < mArgc)
			{
				// We have a parameter //
				t<<"Setting SIM pin to "<<mArgv[arg_idx]<<"\n";
				__instance->setSimPin(mArgv[arg_idx]);
			}
			else
			{
				// Just print the Server name //
				t<<"SIM PIN="<<cNVM::get()->getSimPin()<<"\n";
				return;
			}
		}
		else if(!strcmp("puk",mArgv[arg_idx]))
		{
			// Move onto the parameter//
			arg_idx++;
			if(arg_idx < mArgc)
			{
				// We have a parameter //
				t<<"Setting SIM puk to "<<mArgv[arg_idx]<<"\n";
				__instance->setSimPuk(mArgv[arg_idx]);
			}
			else
			{
				// Just print the Server name //
				t<<"SIM PUK="<<__instance->getSimPuk()<<"\n";
				return;
			}
		}
		else if(!strcmp("port",mArgv[arg_idx]))
		{
			// Move onto the parameter//
			arg_idx++;
			int port;
			if(arg_idx < mArgc)
			{
				// We have a parameter //
				port = atoi(mArgv[arg_idx]);
				t<<"Setting port to "<<port<<"\n";
				__instance->setPort(port);
			}
			else
			{
				// Just print the Server name //
				t<<"PORT="<<(int)__instance->getPort()<<"\n";
				return;
			}
		}
		else if(!strcmp("default",mArgv[arg_idx]))
		{
			// Move onto the parameter//
			t<<"Setting configuration defaults ...\n";
			__instance->setDefault();
		}

		else
		{
			t<<"Unknown parameter "<<mArgv[arg_idx]<<"\n";
		}
		// move onto next param descriptor //
		arg_idx++;
	}

	t<<t.format("%12s","ALRM CELL = ")	<<__instance->getSimCell()<<"\n";
	t<<t.format("%12s","SIM PIN = ")	<<__instance->getSimPin()<<"\n";
	t<<t.format("%12s","SIM PUK = ")	<<__instance->getSimPuk()<<"\n";
	t<<"\n";
	t<<t.format("%12s","APN = ")		<<__instance->getAPN()<<"\n";
	t<<t.format("%12s","USER = ")		<<__instance->getUser()<<"\n";
	t<<t.format("%12s","PASSWD = ")		<<__instance->getPasswd()<<"\n";
	t<<t.format("%12s","SERVER = ")		<<__instance->getServer()<<"\n";
	t<<t.format("%12s","PORT = ")		<<t.format("%04d", __instance->getPort())<<"\n";

}

cNVM::~cNVM()
{
}

