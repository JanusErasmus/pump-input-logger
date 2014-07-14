#include "pwr_mon.h"
#include "log.h"
#include "MCP_rtc.h"
#include "sys_mon.h"


cPwrMon* cPwrMon::_instance = 0;

void cPwrMon::init()
{
	if(!_instance)
	{
		_instance = new cPwrMon();
	}
}

cPwrMon* cPwrMon::get()
{
	return _instance;
}

cPwrMon::cPwrMon()
{
	setupInterrupt();
}

void cPwrMon::setupInterrupt()
{
	cyg_uint32 reg32;
	// Configure EXTI8 to generate interrupts from GPIOD
	// Configure EXTI9 to generate interrupts from GPIOD
	reg32 = 0x33;
	HAL_WRITE_UINT32(CYGHWR_HAL_STM32_AFIO+CYGHWR_HAL_STM32_AFIO_EXTICR3, reg32);

	//enable trigger edge of alarm
	HAL_READ_UINT32( CYGHWR_HAL_STM32_EXTI+CYGHWR_HAL_STM32_EXTI_RTSR, reg32 );
		reg32 |= (1 << 8) | ( 1 << 9 );
		HAL_WRITE_UINT32( CYGHWR_HAL_STM32_EXTI+CYGHWR_HAL_STM32_EXTI_RTSR, reg32 );
		HAL_WRITE_UINT32( CYGHWR_HAL_STM32_EXTI+CYGHWR_HAL_STM32_EXTI_FTSR,reg32);


	//create and mask interrupts
	cyg_interrupt_mask(CYGNUM_HAL_INTERRUPT_EXTI8);
	cyg_interrupt_create(CYGNUM_HAL_INTERRUPT_EXTI8,
			7,
			(cyg_addrword_t)8,
			cPwrMon::handleISR,
			cPwrMon::handleDSR,
			&mPD8_IntHandle,
			&mPD8_Interrupt);

	cyg_interrupt_attach(mPD8_IntHandle);
	cyg_interrupt_unmask(CYGNUM_HAL_INTERRUPT_EXTI8);

	cyg_interrupt_mask(CYGNUM_HAL_INTERRUPT_EXTI9);
		cyg_interrupt_create(CYGNUM_HAL_INTERRUPT_EXTI9,
				7,
				(cyg_addrword_t)9,
				cPwrMon::handleISR,
				cPwrMon::handleDSR,
				&mPD9_IntHandle,
				&mPD9_Interrupt);

		cyg_interrupt_attach(mPD9_IntHandle);
		cyg_interrupt_unmask(CYGNUM_HAL_INTERRUPT_EXTI9);

}

cyg_uint32 cPwrMon::handleISR(cyg_vector_t vector,cyg_addrword_t data)
{
	cyg_interrupt_mask(vector);
	HAL_DELAY_US(50000);
	HAL_DELAY_US(50000);

	HAL_WRITE_UINT32(CYGHWR_HAL_STM32_WWDG + CYGHWR_HAL_STM32_WWDG_CR, 0xFF);

	cyg_interrupt_acknowledge(vector);


	return(CYG_ISR_CALL_DSR);
}

void cPwrMon::handleDSR(cyg_vector_t vector,cyg_uint32 count,cyg_addrword_t data)
{
	//diag_printf("Alarm interrupt %d\n",data);
	cyg_uint8 input  = (cyg_uint8)data - 8;
	switch(input)
	{
	case 0:
	case 1:
	{
		if(!_instance)
			break;

		bool stat = _instance->getPinStat(input);
		diag_printf("Input %d : %s\n", input, stat?"ON":"OFF");


	}
	break;

	default:
		break;

	}

	cyg_interrupt_unmask(vector);
}

bool cPwrMon::getPinStat(cyg_uint8 pin)
{
	cyg_uint32 reg32;
	bool stat = false;

	HAL_READ_UINT32(CYGHWR_HAL_STM32_GPIOD + CYGHWR_HAL_STM32_GPIO_IDR, reg32);
	//diag_printf("0x%08X\n", reg32);

	switch(pin)
	{
	case 0:
		stat = reg32 & (1 << 8);
		break;
	case 1:
		stat = reg32 & (1 << 9);
		break;

	default:
		break;
	}

	return !stat;
}

cPwrMon::~cPwrMon()
{
}

