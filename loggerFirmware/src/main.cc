#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/kernel/diag.h>

#include "init.h"

void * _impure_ptr;	// g++ compatibility

extern "C" void cyg_user_start(void);
void sysResetStatus();


void cyg_user_start(void)
{
	diag_printf("started main thread?\r\n");
	sysResetStatus();

	cInit::init();
}

void sysResetStatus()
{
	cyg_uint32 reg32;

	HAL_READ_UINT32(CYGHWR_HAL_STM32_RCC + CYGHWR_HAL_STM32_RCC_CSR, reg32);

//	if(reg32 & CYGHWR_HAL_STM32_RCC_CSR_PINRSTF)
//	{
//		diag_printf("Reset from NRST pin\n");
//	}

	if(reg32 & CYGHWR_HAL_STM32_RCC_CSR_SFTRSTF)
	{
		diag_printf("Reset from Software\n");
	}
	if(reg32 & CYGHWR_HAL_STM32_RCC_CSR_IWDGRSTF)
	{
		diag_printf("Reset from Independent Watchdog\n");
	}
	if(reg32 & CYGHWR_HAL_STM32_RCC_CSR_WWDGRSTF)
	{
		diag_printf("Reset from Window Watchdog\n");
	}
	if(reg32 & CYGHWR_HAL_STM32_RCC_CSR_LPWRRSTF)
	{
		diag_printf("Reset from Low power \n");
	}


	//Clear reset flags
	HAL_WRITE_UINT32(CYGHWR_HAL_STM32_RCC + CYGHWR_HAL_STM32_RCC_CSR, CYGHWR_HAL_STM32_RCC_CSR_RMVF);
}
