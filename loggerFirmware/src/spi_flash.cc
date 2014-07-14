#include "spi_flash.h"

#define ATMEL_MANUFACTURE_ID		0x1F
#define ATMEL_FAMILY_DENSITY_CODE	0x44

#define FLASH_WREN	0x06
#define FLASH_WRDI	0x04
#define FLASH_RDSR	0x05
#define FLASH_WRSR	0x01
#define FLASH_READ	0x03
#define FLASH_PROG	0x02

#define FLASH_SECT_ERASE	0xD8
#define FLASH_CHIP_ERASE	0xC7
#define FLASH_RDID 			0x9F
#define FLASH_RES			0xAB

#define FLASH_BSY_RDY   1 << 0

SpiFlash * SpiFlash::__instance = NULL;



void SpiFlash::init(cyg_spi_cortexm_stm32_device_t * stm32_dev)
{
   if(__instance == NULL)
   {
      __instance = new SpiFlash(stm32_dev);
   }
}


SpiFlash * SpiFlash::get()
{
   return __instance;
}


SpiFlash::SpiFlash(cyg_spi_cortexm_stm32_device_t * stm32_Device) : mReadyFlag(false)
{
	Spi_Device = &stm32_Device->spi_device;

	SectSize = 0;
	NumSect = 0;
	upperAddr = 0;

	cyg_mutex_init(&busyMutex);

	if(!flash_verify())
	{
		diag_printf("FLASH could not be initialised\n");
		diag_printf("System will restart now\n");
		cyg_thread_delay(1000);
		cyg_uint32 reg32;
		//HAL_READ_UINT32(0xE000ED00 + 0x0C, reg32); //SCB_AIRCR, SYSRESETREQ
		reg32 = (0x5FA << 16) | (1 << 2);
		HAL_WRITE_UINT32(0xE000ED00 + 0x0C, reg32);
		return;
	}

	cyg_uint8 tx_buff[4];
	tx_buff[0] = FLASH_WREN;
	cyg_spi_transfer(Spi_Device,false,1,tx_buff,NULL);

	//global unprotect FLASH
	tx_buff[0] = FLASH_WRSR;
	tx_buff[1] = 0x00;
	cyg_spi_transfer(Spi_Device,false,4,tx_buff,NULL);

	/*//Unprotect a single sector
     tx_buff[0] = 0x39;
     tx_buff[1] = 0x00;
     tx_buff[2] = 0x00;
     tx_buff[3] = 0x00;
     cyg_spi_transfer(Spi_Device,false,4,tx_buff,NULL);*/

	mReadyFlag = true;
}

bool SpiFlash::isReady()
{
	return mReadyFlag;
}

cyg_uint32 SpiFlash::GetSectSize()
{
   return SectSize;
}

cyg_uint32 SpiFlash::GetNumSect()
{
   return NumSect;
}

bool SpiFlash::flash_verify()
{
	cyg_uint8 buff[5];

	//Read device ID
	buff[0] = FLASH_RDID;
	cyg_spi_transfer(Spi_Device,false,5,buff,buff);


	//Ensure the FLASH is from ATMEL
	if(buff[1] == ATMEL_MANUFACTURE_ID)
	{
		//Flash has 4Mbits and sectors of 0x10000
		if(buff[2] == ATMEL_FAMILY_DENSITY_CODE)
		{
			//Now that
			SectSize = 0x10000;
			NumSect = 8;
			upperAddr = (SectSize * NumSect) - 1;

			return true;
		}
	}

	diag_printf("Manufacture ID: 0x%08X\n", buff[1]);
	diag_printf("Device ID1: 0x%08X\n", buff[2]);
	diag_printf("Device ID2: 0x%08X\n", buff[3]);
	diag_printf("Device info: 0x%08X\n", buff[4]);

	return false;
}

bool SpiFlash::ReadSpi(cyg_uint32 addr,cyg_uint8 * data_ptr,cyg_uint32 len)
{
	//Check address out of bounds
	if((addr + len) > upperAddr)
		return false;

	cyg_uint8 cnt = 0;
	cyg_uint8 tx_buff[len+4];
	cyg_uint8 status;

	cyg_mutex_lock(&busyMutex);

	tx_buff[0] = FLASH_READ;
	tx_buff[1] = (addr>>16)&0xFF;
	tx_buff[2] = (addr>>8)&0xFF;
	tx_buff[3] = (addr)&0xFF;
	cyg_spi_transfer(Spi_Device,false,len+4,tx_buff,tx_buff);
	memcpy(data_ptr,&tx_buff[4],len);
	cyg_thread_delay(1);

	//wait for flash to finish
	do
	{
		status = check_status();
		cnt++;
	}while((status & FLASH_BSY_RDY) && (cnt < 20));

	cyg_mutex_unlock(&busyMutex);

	if(status & FLASH_BSY_RDY)
		return false;

	return true;
}

bool SpiFlash::WriteSpi(cyg_uint32 addr, cyg_uint8 * data_ptr, cyg_uint32 len)
{
	//divide write cycles over even 256 byte boundaries
	dbg_printf(1, "writing %d bytes\n", len);

	cyg_mutex_lock(&busyMutex);

	cyg_uint32 Wlen = 0;
	cyg_uint32 BufferLen = len;

	do
	{
		cyg_uint32 SectLen = 0x100 - (addr & 0xFF);
		if(SectLen >= len)
			SectLen = len;
		else if(SectLen == 0)
			SectLen = len - Wlen;

		dbg_printf(1, "Write page @ 0x%08X, len %d\n", addr, SectLen);
		if(!write(addr, &data_ptr[Wlen], SectLen))
		{
			dbg_printf(red, "WRITE ERROR\n");
			cyg_mutex_unlock(&busyMutex);
			return false;
		}

		addr += SectLen;
		Wlen += SectLen;
		len -= SectLen;
	}while(Wlen < BufferLen);

	cyg_mutex_unlock(&busyMutex);

	//verify flash write
	cyg_uint8 buff[len];
	ReadSpi(addr, buff, len);

	if(memcmp(data_ptr, buff, len))
	{
		dbg_printf(red, "WRITE ERROR\n");
		return false;
	}

	return true;
}

bool SpiFlash::write(cyg_uint32 addr,cyg_uint8 * data_ptr,cyg_uint32 len)
{
	//Check address out of bounds
	if((addr + len) > upperAddr)
		return false;

	dbg_dump_buf(2, data_ptr, len);
	cyg_uint8 cnt = 0;
	cyg_uint8 status;
	cyg_uint8 tx_buff[len+4];

	tx_buff[0] = FLASH_WREN;
	cyg_spi_transfer(Spi_Device,false,1,tx_buff,NULL);

	tx_buff[0] = FLASH_PROG;
	tx_buff[1] = (addr>>16)	& 0xFF;
	tx_buff[2] = (addr>>8)	& 0xFF;
	tx_buff[3] = (addr)		& 0xFF;
	memcpy(&tx_buff[4],data_ptr,len);
	cyg_spi_transfer(Spi_Device,false,len+4,tx_buff,NULL);
	cyg_thread_delay(1);

	//wait for flash to finish
	do
	{
		status = check_status();
		cnt++;
	}while((status & FLASH_BSY_RDY) && (cnt < 20));

	if(status & FLASH_BSY_RDY)
		return false;

	return true;
}

bool SpiFlash::flash_erase(cyg_uint32 addr)
{
	//Check address out of bounds
	if(addr > upperAddr)
		return false;

	cyg_uint8 cnt = 0;
	cyg_uint8 tx_buff[4];
	cyg_uint8 status;

	tx_buff[0] = FLASH_WREN;
	cyg_spi_transfer(Spi_Device,false,1,tx_buff,NULL);

	tx_buff[0] = FLASH_SECT_ERASE;
	tx_buff[1] = (addr>>16)&0xFF;
	tx_buff[2] = (addr>>8)&0xFF;
	tx_buff[3] = (addr)&0xFF;
	cyg_spi_transfer(Spi_Device,false,4,tx_buff,NULL);

	//wait for flash to finish
	do
	{
		status = check_status();
		cnt++;
		cyg_thread_delay(50);
	}while((status & FLASH_BSY_RDY) && (cnt < 20));

	if(status & FLASH_BSY_RDY)
		return false;

	return true;
}


cyg_uint8 SpiFlash::check_status()
{
    cyg_uint8 tx_buff[2];
    tx_buff[0] = FLASH_RDSR;
    tx_buff[1] = 0xAA;
    cyg_spi_transfer(Spi_Device,false,2,tx_buff,tx_buff);
    return tx_buff[1];
}



