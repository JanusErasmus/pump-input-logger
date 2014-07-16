#include <cyg/kernel/diag.h>
#include <cyg/kernel/kapi.h>
#include <stdio.h>

#include "var_io.h"
#include "input_port.h"
#include "definitions.h"
#include "sys_mon.h"

cInput* cInput::__instance = NULL;

static const cyg_uint32 interrupt_exti[] =
{
	CYGNUM_HAL_INTERRUPT_EXTI0, //0
	CYGNUM_HAL_INTERRUPT_EXTI1,	//1
	CYGNUM_HAL_INTERRUPT_EXTI2, //2
	CYGNUM_HAL_INTERRUPT_EXTI3,	//3
	CYGNUM_HAL_INTERRUPT_EXTI4,	//4
	CYGNUM_HAL_INTERRUPT_EXTI5,	//5
	CYGNUM_HAL_INTERRUPT_EXTI6,	//6
	CYGNUM_HAL_INTERRUPT_EXTI7,	//7
	CYGNUM_HAL_INTERRUPT_EXTI8,	//8
	CYGNUM_HAL_INTERRUPT_EXTI9,	//9
	CYGNUM_HAL_INTERRUPT_EXTI10,//10
	CYGNUM_HAL_INTERRUPT_EXTI11,//11
	CYGNUM_HAL_INTERRUPT_EXTI12,//12
	CYGNUM_HAL_INTERRUPT_EXTI13,//13
	CYGNUM_HAL_INTERRUPT_EXTI14,//14
	CYGNUM_HAL_INTERRUPT_EXTI15,//15
};


/**
 * Initialize input ports
 *
 * @param portNumbers	A list of the input port numbers PA0 = 0, PA1 = 1 through to PF7 = 87
 * 						Use defined MACROS to generate port numbers INPUTx_(y) where x = A-F and y = 0-7
 * @param portCount		Number of ports in the list
 */
void cInput::init(cyg_uint32* portNumbers, cyg_uint8 portCount)
{
	diag_printf("Initialising cInput\n");

	if(!__instance)
	{
		__instance = new cInput(portNumbers, portCount);
	}
}

cInput* cInput::get()
{
	return __instance;
}

cInput::cInput(cyg_uint32* portNumbers, cyg_uint8 portCount) : mInputCnt(portCount)
{
	mAlarmStart = false;

	mPDx_IntHandle = new cyg_handle_t[mInputCnt];
	mPDx_Interrupt = new cyg_interrupt[mInputCnt];
	mInputList = new cyg_uint32[mInputCnt];
	memcpy(mInputList, portNumbers, sizeof(cyg_uint32) * mInputCnt);

	mQueue = 0;

	setupPorts(mInputList, mInputCnt);
	setupInterrupts(mInputList, mInputCnt);
}

void cInput::setupPorts(cyg_uint32* ports, cyg_uint8 count)
{
	cyg_uint32 port, pin, pinspec;

	for (int k = 0; k < count; k++)
	{
		cyg_uint32 gpio_num = ports[k];
		pin = gpio_num & 0xF;
		port = gpio_num >> 4;
		//diag_printf("Setup port%d pin%d\n", port, pin);

		// Generate the pin setup specification and configure it.
		pinspec = STM32_GPIO_PINSPEC (port, pin, IN, PULLUP);
		CYGHWR_HAL_STM32_GPIO_SET (pinspec);
	}
}

void cInput::setupInterrupts(cyg_uint32* ports, cyg_uint8 count)
{
	cyg_uint32 port, pin;

	//create and mask interrupts for all the buttons of the display
	for(int k = 0; k < mInputCnt; k++)
	{
		cyg_uint32 gpio_num = ports[k];
		pin = gpio_num & 0xF;
		port = gpio_num >> 4;

		enableInterrupt(port, pin);
		cyg_uint32 exti = interrupt_exti[pin];

		cyg_interrupt_mask(exti);
		cyg_interrupt_create(exti,
				7,
				(cyg_addrword_t)k,
				cInput::handleISR,
				cInput::handleDSR,
				&mPDx_IntHandle[k],
				&mPDx_Interrupt[k]);

		cyg_interrupt_attach(mPDx_IntHandle[k]);
		cyg_interrupt_unmask(exti);
	}
}

void cInput::enableInterrupt(cyg_uint8 port, cyg_uint8 pin)
{
	cyg_uint32 exticr = 0;
	cyg_uint8 shiftLeft = 0;
	switch(pin)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		exticr = CYGHWR_HAL_STM32_AFIO+CYGHWR_HAL_STM32_AFIO_EXTICR1;
		shiftLeft = (pin & 0x0F) * 4;
		break;
	case 4:
	case 5:
	case 6:
	case 7:
		exticr = CYGHWR_HAL_STM32_AFIO+CYGHWR_HAL_STM32_AFIO_EXTICR2;
		shiftLeft = ((pin - 4) & 0x0F) * 4;
			break;
	case 8:
	case 9:
	case 10:
	case 11:
		exticr = CYGHWR_HAL_STM32_AFIO+CYGHWR_HAL_STM32_AFIO_EXTICR3;
		shiftLeft = ((pin - 8) & 0x0F) * 4;
			break;
	case 12:
	case 13:
	case 14:
	case 15:
		exticr = CYGHWR_HAL_STM32_AFIO+CYGHWR_HAL_STM32_AFIO_EXTICR4;
		shiftLeft = ((pin - 12) & 0x0F) * 4;
			break;
	default:
		break;
	}

	if(exticr)
	{
		cyg_uint32 reg32;
		HAL_READ_UINT32(exticr, reg32);
		reg32 |= port << shiftLeft;
		HAL_WRITE_UINT32(exticr, reg32);
	}
}

void cInput::start()
{
	cyg_uint32 reg32;
	//enable trigger edge of interrupt
	HAL_READ_UINT32( CYGHWR_HAL_STM32_EXTI+CYGHWR_HAL_STM32_EXTI_RTSR, reg32 );


	for (int k = 0; k < mInputCnt; k++)
	{
		cyg_uint32 gpio_num = mInputList[k];
		cyg_uint32 pin = gpio_num & 0xF;
		reg32 |= 1 << pin;
	}


	HAL_WRITE_UINT32( CYGHWR_HAL_STM32_EXTI+CYGHWR_HAL_STM32_EXTI_RTSR, reg32 );
	HAL_WRITE_UINT32( CYGHWR_HAL_STM32_EXTI+CYGHWR_HAL_STM32_EXTI_FTSR,reg32);

	//wait a while for interrupts to stabilise
	cyg_thread_delay(500);
}

cyg_uint32 cInput::handleISR(cyg_vector_t vector, cyg_addrword_t data)
{
    cyg_interrupt_mask(vector);

   HAL_DELAY_US(20000);

    cyg_interrupt_acknowledge(vector);


   	return(CYG_ISR_CALL_DSR);

}

void cInput::handleDSR(cyg_vector_t vector,cyg_uint32 count,cyg_addrword_t data)
{
	//diag_printf("Alarm interrupt %d\n",data + 1);
	cyg_uint8 input  = (cyg_uint8)data;

	int stat = __instance->getPortState(input);

	if(__instance->mQueue)
	{
		cActionQueue::s_event *evt = new cActionQueue::s_event(input, stat);
		cActionQueue::s_action * act = new cActionQueue::s_action(cActionQueue::event, (cyg_addrword_t)evt);

		__instance->mQueue->QAction(act);
	}

	cyg_interrupt_unmask(vector);
}

bool cInput::getPortState(cyg_uint8 input)
{
	cyg_uint32 reg32;
	bool stat = false;

	if(input >= mInputCnt)
		return false;

	cyg_uint32 port, pin;
	pin = mInputList[input] & 0xF;
	port = mInputList[input] >> 4;

	HAL_READ_UINT32(stm32_gpio_port_registers[port] + CYGHWR_HAL_STM32_GPIO_IDR, reg32);
	stat = reg32 & (1 << pin);

	return !stat;
}

void cInput::updateAlarmLED(cyg_uint8 pin)
{
	cyg_uint8 stat = getPortState(pin);
	cLED::get()->showIO(pin + 1, stat);
}

void cInput::showInputs(cTerm & t,int argc,char *argv[])
{
	t<<YELLOW("INPUT status:\n");
	for(cyg_uint8 k = 0; k < __instance->mInputCnt; k++)
	{
		if( __instance->getPortState(k))
			t<<t.format(GREEN("\tP%d: 1\n"), k);
		else
			t<<t.format(RED("\tP%d: 0\n"), k);
	}
}
