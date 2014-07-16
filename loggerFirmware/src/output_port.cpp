#include <cyg/kernel/diag.h>
#include <stdio.h>
#include <stdlib.h>

#include "output_port.h"
#include "definitions.h"
#include "nvm.h"

cOutput* cOutput::__instance = NULL;

/**
 * Initialize output ports
 *
 * @param portNumbers	A list of the output port numbers PA0 = 0, PA1 = 1 through to PF7 = 87
 * 						Use defined MACROS of input_port.h to generate port numbers INPUTx_(y) where x = A-F and y = 0-7
 * @param portCount		Number of ports in the list
 */
void cOutput::init(cyg_uint32* portNumbers, cyg_uint8 portCount)
{
	diag_printf("Initialising cOutput\n");
	if(!__instance)
	{
		__instance = new cOutput(portNumbers, portCount);
	}
}

cOutput* cOutput::get()
{
	return __instance;
}


cOutput::cOutput(cyg_uint32* portNumbers, cyg_uint8 portCount) : mOutputCnt(portCount)
{
	mOutputList = new cyg_uint32[mOutputCnt];
	memcpy(mOutputList, portNumbers, sizeof(cyg_uint32) * mOutputCnt);

	setupPorts(mOutputList, mOutputCnt);

	//set Relays to defalt value
	for (int k = 0; k < mOutputCnt; ++k)
	{
		updateOutput(k,cNVM::get()->getOutputStat(k));
	}

}

void cOutput::setupPorts(cyg_uint32* ports, cyg_uint8 count)
{
	cyg_uint32 port, pin, pinspec;

	for (int k = 0; k < count; k++)
	{
		cyg_uint32 gpio_num = ports[k];
		pin = gpio_num & 0xF;
		port = gpio_num >> 4;

		// Generate the pin setup specification and configure it.
		pinspec = STM32_GPIO_PINSPEC (port, pin, OUT_2MHZ, GPOPP);
		CYGHWR_HAL_STM32_GPIO_SET (pinspec);
	}
}

bool cOutput::setPortState(cyg_uint8 num, bool state)
{
	if(num > mOutputCnt)
		return false;

	updateOutput(num, state);

	return true;
}

void cOutput::updateOutput(cyg_uint8 num, bool state)
{
	if(num > mOutputCnt)
		return;

	cyg_uint32 port, pin, pinspec;
	pin = mOutputList[num] & 0xF;
	port = mOutputList[num] >> 4;
	pinspec = STM32_GPIO_PINSPEC (port, pin, OUT_2MHZ, GPOPP);
	CYGHWR_HAL_STM32_GPIO_OUT (pinspec, state);

}


bool cOutput::getPortState(cyg_uint8 num)
{
	cyg_uint32 port, pin, reg32;

		pin = mOutputList[num] & 0xF;
		port = mOutputList[num] >> 4;

		HAL_READ_UINT32(stm32_gpio_port_registers[port] + CYGHWR_HAL_STM32_GPIO_IDR, reg32);

		return (reg32 & (1 << pin));
}


void  cOutput::showOutputs(cTerm & t,int argc,char *argv[])
{
	t<<YELLOW("OUTPUT status:\n");
	for(cyg_uint8 k = 0; k < __instance->mOutputCnt; k++)
	{
		if( __instance->getPortState(k))
			t<<t.format(GREEN("\tO%d: 1\n"), k);
		else
			t<<t.format(RED("\tO%d: 0\n"), k);
	}
}

void cOutput::setOutput(cTerm & t,int argc,char *argv[])
{
	if(argc  > 2)
	{
		cyg_uint8 port = strtoul(argv[1],NULL,16);
		bool stat = strtoul(argv[2],NULL,16);

		if(stat)
			t<<t.format(YELLOW("SET relay %d\n"), port);
		else
			t<<t.format(CYAN("RESET relay %d\n"), port);

		__instance->setPortState(port, stat);
	}
	else
	{
		t<<t.format(RED("Enter relay number and state:\n"));
	}
}
