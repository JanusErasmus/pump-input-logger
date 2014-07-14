#include <cyg/kernel/diag.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/var_io.h>
#include <stdio.h>

#include "MCP_rtc.h"


//MCP commands
#define MCP_SR		0xFF
#define MCP_CONF	0x03
#define MCP_CR		0x07


cRTC* cRTC::__instance = 0;

void cRTC::init()
{
	if(!__instance)
	{
		__instance = new cRTC();
	}
}

cRTC* cRTC::get()
{
	return __instance;
}



cRTC::cRTC()
{	
	diag_printf("MCP7941x created\n");

	cyg_uint32 reg32;

	//HAL_READ_UINT32(CYGHWR_HAL_STM32_RCC + CYGHWR_HAL_STM32_RCC_CFGR, reg32);
	// diag_printf("RCC_CFGR: 0x%08X\n", reg32);
	//HAL_READ_UINT32(CYGHWR_HAL_STM32_RCC+CYGHWR_HAL_STM32_RCC_APB1ENR, reg32);
	// diag_printf("RCC_APB1ENR: 0x%08X\n", reg32);

	//Reset I2C peripheral
	//HAL_WRITE_UINT32(CYGHWR_HAL_STM32_I2C1 + CYGHWR_HAL_STM32_I2C_CR1, CYGHWR_HAL_STM32_I2C_CR1_SWRST);

	//Clock control
	reg32 =  CYGHWR_HAL_STM32_I2C_CCR_CCR(0x14);
	HAL_WRITE_UINT32(CYGHWR_HAL_STM32_I2C1 + CYGHWR_HAL_STM32_I2C_CCR, reg32);

	//HAL_READ_UINT32(CYGHWR_HAL_STM32_I2C1 + CYGHWR_HAL_STM32_I2C_CCR, reg32);
	// diag_printf("CCR: 0x%08X\n", reg32);

	//Rise Time
	reg32 = 1;
	HAL_WRITE_UINT32(CYGHWR_HAL_STM32_I2C1 + CYGHWR_HAL_STM32_I2C_TRISE, reg32);
	//HAL_READ_UINT32(CYGHWR_HAL_STM32_I2C1 + CYGHWR_HAL_STM32_I2C_TRISE, reg32);
	// diag_printf("RISE: 0x%08X\n", reg32);

	//enable I2C peripheral
	HAL_WRITE_UINT32(CYGHWR_HAL_STM32_I2C1 + CYGHWR_HAL_STM32_I2C_CR1, CYGHWR_HAL_STM32_I2C_CR1_PE);


	//Input clock
	reg32  = CYGHWR_HAL_STM32_I2C_CR2_FREQ(6);
	HAL_WRITE_UINT32(CYGHWR_HAL_STM32_I2C1 + CYGHWR_HAL_STM32_I2C_CR2, reg32);

	//HAL_READ_UINT32(CYGHWR_HAL_STM32_I2C1 + CYGHWR_HAL_STM32_I2C_CR2, reg32);
	// diag_printf("CR2: 0x%08X\n", reg32);


	HAL_READ_UINT32(CYGHWR_HAL_STM32_I2C1 + CYGHWR_HAL_STM32_I2C_SR1, reg32);
	//diag_printf("SR1: 0x%08X\n", reg32);
	HAL_READ_UINT32(CYGHWR_HAL_STM32_I2C1 + CYGHWR_HAL_STM32_I2C_SR2, reg32);
	//diag_printf("SR2: 0x%08X\n", reg32);


	mI2CcBus.i2c_init_fn    = cyg_stm32_i2c_init;
	mI2CcBus.i2c_tx_fn      = cyg_stm32_i2c_tx;
	mI2CcBus.i2c_rx_fn      = cyg_stm32_i2c_rx;
	mI2CcBus.i2c_stop_fn    = cyg_stm32_i2c_stop;
	mI2CcBus.i2c_extra      = NULL;


	cyg_stm32_i2c_init(&mI2CcBus); //init mutex and waiter here...

	i2c = new cyg_i2c_device();
	i2c->i2c_bus =  &mI2CcBus;
	i2c->i2c_address = 0xDE ;
	i2c->i2c_flags = 0x00;
	i2c->i2c_delay = 1000;

	 mValidTime = false;
	 mPowerDown = 0;
	 mPowerUp = 0;

	setupRTC();
}

void cRTC::setupRTC()
{
	cyg_uint8 stat;
	read(MCP_CONF,&stat,1);
	//diag_dump_buf(&stat,1);

	//check if OSC has been started
	if(!(stat & 0x20))
	{
		//start external OSC
		cyg_uint8 STbit = 0x80;
		write(0x00,&STbit, 1);
		diag_printf("OSC enabled\n");
	}
	else //if(stat & 0x10) //check if Vbat was there to keep time
	{
		mValidTime = true;
	}

	if(mValidTime)
	{
		syncTime();

		//read time stamp values
		mPowerUp = readPowerUp();
		mPowerDown = readPowerDown();
	}

	//Clear time stamp values
	cyg_uint8 Vbat = 0x08;
	write(MCP_CONF, &Vbat, 1);
	//	diag_printf("Vbat enabled\n");

}

bool cRTC::status()
{
	cyg_uint8 stat;

	if(!read(MCP_SR, &stat,1))
		return false;

	diag_printf("MCP7941x status\n ");

	diag_printf("0x%02X \n", stat);

	return true;

}

void cRTC::setTime(cyg_uint8 yy, cyg_uint8 mm, cyg_uint8 dd, cyg_uint8 h, cyg_uint8 m, cyg_uint8 s)
{

	yy -= 2000;

	cyg_uint8 buff[7];
	buff[0] = 0x80 | (s/10 << 4) | (s % 10);
	buff[1] = (m/10 << 4) | (m % 10);
	buff[2] = ((h/10 << 4) & 0x30) | (h % 10);
	buff[3] = 0x08;
	buff[4] = ((dd/10 << 4) & 0x30) | (dd % 10);
	buff[5] = ((mm/10 << 4) & 0x10) | (mm % 10);
	buff[6] = (yy/10 << 4) | (yy % 10);
	write(0x00, buff, 7);

	mValidTime = true;

	dbg_dump_buf(1, buff,7);
}

void cRTC::setTime(time_t* t)
{
	struct tm* tStruct = localtime(t);

	diag_printf("%d-", tStruct->tm_year + 1900);
	diag_printf("%02d-", tStruct->tm_mon + 1);
	diag_printf("%02d ", tStruct->tm_mday);
	diag_printf("%02d:", tStruct->tm_hour);
	diag_printf("%02d:", tStruct->tm_min);
	diag_printf("%02d\n", tStruct->tm_sec);

	setTime(tStruct->tm_year + 1900, tStruct->tm_mon + 1, tStruct->tm_mday, tStruct->tm_hour, tStruct->tm_min, tStruct->tm_sec);

	mValidTime = true;
	syncTime();
}

time_t cRTC::timeNow()
{
	if(!mValidTime)
		return 0;

	cyg_uint8 stat[7];
	read(0x00,stat,7);
	dbg_dump_buf(1, stat,7);

	cyg_uint8 yy,mm,dd,h,m,s;

	s =  ((((stat[0] >> 4) & 0x07) * 10 ) + (stat[0] & 0x0F));
	m =  ((( stat[1] >> 4)         * 10 ) + (stat[1] & 0x0F));
	h =  ((((stat[2] >> 4) & 0x03) * 10 ) + (stat[2] & 0x0F));
	dd = ((((stat[4] >> 4) & 0x03) * 10 ) + (stat[4] & 0x0F));
	mm = ((((stat[5] >> 4) & 0x01) * 10 ) + (stat[5] & 0x0F));
	yy = ((((stat[6] >> 4) & 0x0F) * 10 ) + (stat[6] & 0x0F));

	dbg_printf(1, "Time now: %02d-%02d-%02d  ", yy, mm, dd);
	dbg_printf(1, "%02d:%02d:%02d\n", h, m, s);

	struct tm info;
	info.tm_year = 100 + yy;
	info.tm_mon =  mm - 1;
	info.tm_mday = dd;
	info.tm_hour = h;
	info.tm_min = m;
	info.tm_sec = s;

	return mktime(&info);
}

time_t cRTC::readPowerDown()
{
	cyg_uint8 stat[4];
	read(0x18,stat,4);
	dbg_dump_buf(1, stat,4);

	//if all zero's it was a system reset without power down
	if(stat[0] == 0 && stat[1] == 0 && stat[2] == 0 && stat[3] == 0)
		return 0;

	cyg_uint8 yy, mm,dd,h,m;

	m =  ((( stat[0] >> 4)         * 10 ) + (stat[0] & 0x0F));
	h =  ((((stat[1] >> 4) & 0x03) * 10 ) + (stat[1] & 0x0F));
	dd = ((((stat[2] >> 4) & 0x03) * 10 ) + (stat[2] & 0x0F));
	mm = ((((stat[3] >> 4) & 0x01) * 10 ) + (stat[3] & 0x0F));

	time_t now = time(NULL);
	yy = localtime(&now)->tm_year;


	dbg_printf(1, "Time now: %02d-%02d-%02d  ", yy, mm, dd);
	dbg_printf(1, "%02d:%02d:%02d\n", h, m, 0);

	struct tm  info;

	info.tm_year = yy;
	info.tm_mon =  mm - 1;
	info.tm_mday = dd;
	info.tm_hour = h;
	info.tm_min = m;
	info.tm_sec = 0;

	return mktime(&info);
}



time_t cRTC::readPowerUp()
{
	cyg_uint8 stat[4];
	read(0x1C,stat,4);
	dbg_dump_buf(1, stat,4);

	//if all zero's it was a system reset without power down
	if(stat[0] == 0 && stat[1] == 0 && stat[2] == 0 && stat[3] == 0)
		return 0;

	cyg_uint8 yy, mm,dd,h,m;

	m =  ((( stat[0] >> 4)         * 10 ) + (stat[0] & 0x0F));
	h =  ((((stat[1] >> 4) & 0x03) * 10 ) + (stat[1] & 0x0F));
	dd = ((((stat[2] >> 4) & 0x03) * 10 ) + (stat[2] & 0x0F));
	mm = ((((stat[3] >> 4) & 0x01) * 10 ) + (stat[3] & 0x0F));

	time_t now = time(NULL);
	yy = localtime(&now)->tm_year;


	dbg_printf(1, "Time now: %02d-%02d-%02d  ", yy, mm, dd);
	dbg_printf(1, "%02d:%02d:%02d\n", h, m, 0);

	struct tm  info;

	info.tm_year = yy;
	info.tm_mon =  mm - 1;
	info.tm_mday = dd;
	info.tm_hour = h;
	info.tm_min = m;
	info.tm_sec = 0;

	return mktime(&info);
}



void cRTC::syncTime()
{
	time_t t = this->timeNow();
	cyg_libc_time_settime(t);
}

cyg_uint8 cRTC::read(cyg_uint8 addr, cyg_uint8* buff, cyg_uint8 len)
{
	//diag_printf("Reading... 0x%02X\n", addr);

	cyg_i2c_transaction_tx(i2c, true, &addr, 1, false);
	return cyg_i2c_rx(i2c,buff,len);


}

cyg_uint8 cRTC::write(cyg_uint8 addr, cyg_uint8* buff, cyg_uint8 len)
{
	//diag_printf("Reading... 0x%02X\n", addr);

	cyg_uint8 tmp[len + 1];
	tmp[0] = addr;
	memcpy(&tmp[1], buff, len);

	return cyg_i2c_transaction_tx(i2c, true, tmp, len +1 , false);
}

time_t cRTC::getPowerDown()
{
	return mPowerDown;
}



time_t cRTC::getPowerUp()
{
	return mPowerUp;
}



