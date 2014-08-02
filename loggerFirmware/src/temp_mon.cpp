#include <stdio.h>

#include "utils.h"
#include "temp_mon.h"
#include "var_io.h"
#include "event.h"
#include "log.h"
#include "MCP_rtc.h"
#include "nvm.h"
#include "fuelLookup.h"

cTempMon* cTempMon::_instance = 0;

void cTempMon::init()
{
	if(!_instance)
	{
		_instance = new cTempMon();
	}
}


cTempMon* cTempMon::get()
{
	return _instance;
}

cTempMon::cTempMon()
{

	cyg_mutex_init(&mSampleMutex);

	cyg_uint32 reg32;
	HAL_READ_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_CR2, reg32);
	reg32 = CYGHWR_HAL_STM32_ADC_CR2_TSVREFE | //enable temp sensor
			CYGHWR_HAL_STM32_ADC_CR2_ADON |		//switch on
			CYGHWR_HAL_STM32_ADC_CR2_EXTTRIG | //enable External trigger
			CYGHWR_HAL_STM32_ADC_CR2_EXTSEL(0x07) | //SWSTART
			0;
	HAL_WRITE_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_CR2, reg32);

	HAL_READ_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_CR2, reg32);
	//diag_printf("CR2: 0x%08X\n", reg32);

	//Reset calibration
	HAL_READ_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_CR2, reg32);
	reg32 |= CYGHWR_HAL_STM32_ADC_CR2_RSTCAL;
	HAL_WRITE_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_CR2, reg32);
	do
	{
		HAL_READ_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_CR2, reg32);
		//diag_printf("1");
	}
	while(reg32 & CYGHWR_HAL_STM32_ADC_CR2_RSTCAL);

	//Calibrate
	HAL_READ_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_CR2, reg32);
	reg32 |= CYGHWR_HAL_STM32_ADC_CR2_CAL;
	HAL_WRITE_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_CR2, reg32);
	do
	{
		HAL_READ_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_CR2, reg32);
		//diag_printf("2");
	}
	while(reg32 & CYGHWR_HAL_STM32_ADC_CR2_CAL);

	//set sample time AN8 AN9
	HAL_READ_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_SMPR2, reg32);
	reg32 |= CYGHWR_HAL_STM32_ADC_SMPR2_SMP8(0x07)| //longest sample time
			CYGHWR_HAL_STM32_ADC_SMPR2_SMP9(0x07)| //longest sample time
			0;
	HAL_WRITE_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_SMPR2, reg32);
	//set sample time AN15
	HAL_READ_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_SMPR1, reg32);
	reg32 |= 	CYGHWR_HAL_STM32_ADC_SMPR1_SMP14(0x03)| //longest sample time
				CYGHWR_HAL_STM32_ADC_SMPR1_SMP15(0x07)| //longest sample time
				CYGHWR_HAL_STM32_ADC_SMPR1_SMP16(0x03)| //longest sample time
				CYGHWR_HAL_STM32_ADC_SMPR1_SMP17(0x03)| //longest sample time
			0;
	HAL_WRITE_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_SMPR1, reg32);

	//enable discontinuous mode
	HAL_READ_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_CR1, reg32);
	reg32 = CYGHWR_HAL_STM32_ADC_CR1_DISCEN |
			CYGHWR_HAL_STM32_ADC_CR1_DISCNUM(0x00) |
			0;
	HAL_WRITE_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_CR1, reg32);

	//select 4 channels to sample
	HAL_READ_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_SQR1, reg32);
	reg32 = CYGHWR_HAL_STM32_ADC_SQR1_L(0x03);
	HAL_WRITE_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_SQR1, reg32);

	//set sample sequence
	HAL_READ_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_SQR3, reg32);
	reg32 |=CYGHWR_HAL_STM32_ADC_SQR3_SQ1(15) |	//select channel 9 as 1st sequence
			CYGHWR_HAL_STM32_ADC_SQR3_SQ2(8) |	//select channel 8 as 2nd sequence
			CYGHWR_HAL_STM32_ADC_SQR3_SQ3(9) |
			CYGHWR_HAL_STM32_ADC_SQR3_SQ4(14) |
		//	CYGHWR_HAL_STM32_ADC_SQR3_SQ4(16) |
		//	CYGHWR_HAL_STM32_ADC_SQR3_SQ5(17) |
			0;
	HAL_WRITE_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_SQR3, reg32);
}

/**Check the temperatures and if the changed log events.
 *
 * @return changeStatus If the status of any port has changed from crytical to good
 * 			or vice versa checkTemps() will return true
 */
cyg_bool cTempMon::checkTemps()
{
	cyg_bool changeStatus = false;
	float temps[5];
	static eTempState tempStatus[4] = {good, good, good, good};

	if(!getSample(temps))
		return false;

	for(int k = 0; k < AN_IN_CNT; k++)
	{
		//set these sampled values if changed
		setSample(k, temps[k]);

		//check if a temperature is critical, return true if any of the port states has changed
		if(isCritical(k, temps[k]))
		{
			if(tempStatus[k] == good) //previous state was good, it is now critical
			{
				changeStatus = true; //a port state has changed
				tempStatus[k] = critical;
			}
		}
		else
		{
			if(tempStatus[k] == critical) //previous state was critical, it is now good
			{
				changeStatus = true; //a port state has changed
				tempStatus[k] = good;
			}
		}
	}

	return changeStatus;
}

void cTempMon::setSample(cyg_uint8 port, float val)
{
	if(port >= 4)
		return;

	float prevSample = ANsamples[port].get().value;
	float range = cNVM::get()->getSampleRange(port);

	if((prevSample - range) > val || val > (prevSample + range))
	{
		ANsamples[port].set(val);

		cEvent e(port, val, cRTC::get()->timeNow());
		e.showEvent();
		cLog::get()->logEvent(&e);

	}
}

cyg_bool cTempMon::isCritical(cyg_uint8 port, float val)
{
//TODO	float upper = cNVM::get()->getUpperLimit(port);
//	float lower = cNVM::get()->getLowerLimit(port);
//
//	if((upper < val) || (val < lower) )
//	{
//		printf("Port%d: %0.1f Critical %0.1f < temp < %0.1f\n", port, val, upper, lower);
//		return true;
//	}

	return false;
}

void cTempMon::logSamplesNow()
{
	float temps[5];

	if(!getSample(temps))
		return;

	for(int k = 0; k < AN_IN_CNT; k++)
	{
		ANsamples[k].set(temps[k]);

		cEvent e(k, temps[k], cRTC::get()->timeNow());
		e.showEvent();
		cLog::get()->logEvent(&e);
	}
}

bool cTempMon::getSample(float* buff)
{
	cyg_mutex_lock(&mSampleMutex);

	bool stat = false;

#ifdef 	FILTER_SAMPLES_CNT
	stat = getFilteredSample(buff);
#else
	stat = getSamplesOnce(buff);
#endif

	cyg_mutex_unlock(&mSampleMutex);

	return stat;
}

bool cTempMon::getSamplesOnce(float* buff)
{
	if(!cRTC::get()->timeNow()  || !buff)
		return false;

	for(cyg_uint8 port = 0; port < 4; port++)
	{
		cyg_uint16 s = sample();

		switch(port)
		{
			//port 0 - 2 is temperature
			case 0:
			case 1:
			case 2:
				buff[port] = ((float)s * 0.0805664) - 273.00 ; // ({[(s * 3.3 / 2^12)] - 2.98 } / 10e-3 ) + 25
				break;

			//port 3 is fuel level
			case 3:
			{
				//Use the lookup table
				float fuel = lookupFuel(s);

				//Use a linear approach
//				float fuel = ((float)s * -0.0609756) + 125.1829;
//
//				if(fuel > 100)
//					fuel = 100;
//
//				if(fuel < 0)
//					fuel = 0;

//				printf("fuel:%5d %.4f\n", s, fuel);
				buff[port] = fuel;
			}
				break;

			default:
				buff[port] = (float)s;
				break;
				//diag_printf("%d: 0x%08X\n", k, s);
		}

	}

	return true;
}

bool cTempMon::getFilteredSample(float* buff)
{
	if(!cRTC::get()->timeNow()  || !buff)
		return false;

	//sample the 4 ports for FILTER_SAMPLES_CNT samples
	cyg_uint16 portSamples[4][FILTER_SAMPLES_CNT]; //[port number][sample number]

	for(cyg_uint8 k = 0; k < FILTER_SAMPLES_CNT; k++)
	{
		for(cyg_uint8 port = 0; port < 4; port++)
		{
			portSamples[port][k] = sample();
			cyg_thread_delay(125);
		}
	}

	for(cyg_uint8 port = 0; port < 4; port++)
	{
		//diag_printf("port %d:\n",port);
		cyg_uint16 s = filter_movingAverage(portSamples[port], FILTER_SAMPLES_CNT);

		switch(port)
		{
			//port 0 - 2 is temperature
			case 0:
			case 1:
			case 2:
				buff[port] = ((float)s * 0.0805664) - 273.00 ; // ({[(s * 3.3 / 2^12)] - 2.98 } / 10e-3 ) + 25
				break;

				//port 3 is fuel level
			case 3:
			{
				//Use the lookup table
				float fuel = lookupFuel(s);

				//Use a linear approach
				//				float fuel = ((float)s * -0.0609756) + 125.1829;
				//
				//				if(fuel > 100)
				//					fuel = 100;
				//
				//				if(fuel < 0)
				//					fuel = 0;

				//				printf("fuel:%5d %.4f\n", s, fuel);
				buff[port] = fuel;
			}
			break;

			default:
				buff[port] = (float)s;
				break;
				//diag_printf("%d: 0x%08X\n", k, s);
		}
	}


	return true;
}

cyg_uint16 cTempMon::filter_movingAverage(cyg_uint16* samples, cyg_uint16 count)
{
	if(!samples)
		return 0;

	float alpha = 0.5;//0.142857; n = 5
	float y0 = samples[0];
	float y1 = 0;

	for(cyg_uint16 k = 1; k < count; k++)
	{
		y1 = (alpha * y0) + (1 - alpha) * (float)samples[k];
		y0 = y1;
		//diag_printf("\t%d: %d %d\n",k, samples[k] ,(cyg_uint16)y0);
	}

	return (cyg_uint16)y1;
}

float cTempMon::lookupFuel(cyg_uint16 level)
{
	cyg_uint8 k = 0;
	while(fuelLevel[k])
	{
		if(level >= fuelLevel[k])
			break;

		k++;
	}

	if(k > 128)
		return 100;

	return fuelPersentage[k];
}

cyg_uint16 cTempMon::sample()
{
	cyg_uint32 reg32;

	//clear status register
	HAL_WRITE_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_SR, 0x00);

	//sample
	HAL_READ_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_CR2, reg32);
	reg32 |= CYGHWR_HAL_STM32_ADC_CR2_SWSTART;
	HAL_WRITE_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_CR2, reg32);
	do
	{
		HAL_READ_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_SR, reg32);
	}
	while(!(reg32 & CYGHWR_HAL_STM32_ADC_SR_EOC));

	HAL_READ_UINT32( CYGHWR_HAL_STM32_ADC1 + CYGHWR_HAL_STM32_ADC_DR, reg32);
	//diag_printf("sampled1 0x%08X\n", reg32);

	return (reg32 & 0xFFFF);
}

void cTempMon::showMeasurements()
{
	printf("\tAN1:\t\t\tAN2:\t\t\tAN3:\t\t\tFuel:\n");

		analogSeq::seqSample* buffT1 = ANsamples[0].getBuff();
		analogSeq::seqSample* buffT2 = ANsamples[1].getBuff();
		analogSeq::seqSample* buffT3 = ANsamples[2].getBuff();
		analogSeq::seqSample* buffT4 = ANsamples[3].getBuff();

		char timeStr[16];
		for(int k = 0; k < SEQ_BUFFER_SIZE; k++)
		{
			getTimeString(buffT1[k].time, timeStr);
			if(analogSeq::isNaN(buffT1[k].value))
				printf("%5d: %s %s %cC", buffT1[k].sequence, timeStr, "nan " , 0xF8); 	//column1
			else
				printf("%5d: %s %.1f %cC", buffT1[k].sequence, timeStr, buffT1[k].value , 0xF8); 	//column1

			getTimeString(buffT2[k].time, timeStr);
			if(analogSeq::isNaN(buffT2[k].value))
				printf("%5d: %s %s %cC", buffT2[k].sequence, timeStr, "nan " , 0xF8); 	//column2
			else
				printf("%5d: %s %.1f %cC", buffT2[k].sequence, timeStr, buffT2[k].value , 0xF8); 	//column2

			getTimeString(buffT3[k].time, timeStr);
			if(analogSeq::isNaN(buffT3[k].value))
				printf("%5d: %s %s %cC", buffT3[k].sequence, timeStr, "nan " , 0xF8); 	//column4
			else
				printf("%5d: %s %.1f %cC", buffT3[k].sequence, timeStr, buffT3[k].value , 0xF8); 	//column3

			getTimeString(buffT4[k].time, timeStr);
			if(analogSeq::isNaN(buffT4[k].value))
				printf("%5d: %s %s %%", buffT4[k].sequence, timeStr, "nan "); 	//column4
			else
				printf("%5d: %s %.1f %%", buffT4[k].sequence, timeStr, buffT4[k].value ); 	//column4

			printf("\n");
		}
		printf("\n");
}

void cTempMon::getTimeString(time_t t, char* str)
{
	char* argv[5];
	int argc = 5;
	util_parse_params(ctime(&t), argv, argc,' ', ' ');
	strcpy(str, argv[3]);
}

cTempMon::~cTempMon()
{
}

