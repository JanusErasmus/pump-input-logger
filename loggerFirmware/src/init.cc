#include <cyg/kernel/diag.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/kernel/kapi.h>
#include <stdlib.h>
#include <stdio.h>

#include "definitions.h"
#include "init.h"
#include "term.h"
#include "spi_flash.h"
#include "spi_dev.h"
#include "nvm.h"
#include "MCP_rtc.h"
#include "led.h"
#include "pwr_mon.h"
#include "temp_mon.h"
#include "modem.h"
#include "log.h"
#include "hobbs_timer.h"
#include "sys_mon.h"
#include "config.h"

cInit * cInit::__instance = NULL;

/**
 * This function is the system init function.
 * It is conctructed in such a way as to be only
 * execute the constructor once, thus only initialising
 * the system once
 *
 * @example cInit::init();
 */
void cInit::init()
{
    if (__instance == NULL) //Allow instance to be creater only once.
    {
        __instance = new cInit();
    }
}
/**
 * The private default constructor.
 * The constructor is not callable from outside the class
 * and starts the show by creating the first system thread
 */
cInit::cInit()
{
    diag_printf("Init class created ...\n");

    cyg_thread_create(INIT_PRIOR,
                      cInit::init_thread_func,
                      (cyg_addrword_t)this,
                      (char *)"Init Thread",
                      mStack,
                      INIT_STACK_SIZE,
                      &mThreadHandle,
                      &mThread);
    cyg_thread_resume(mThreadHandle);
}
/**ty
 * The main thread function for the system. The whole show
 * gets started from this.
 *
 * @param arg    Pointer to the instance of the cInit class
 */
void cInit::init_thread_func(cyg_addrword_t arg)
{
	diag_printf("Init thread\n");
	cInit * i = (cInit *)arg;

	i->init_system();


	cyg_thread_delay(50);


	// Initialise the Terminal
	cTerm::init((char *)"/dev/ttydiag",128,"inputMon>>");

	for (;;)
	{
		cLED::get()->animate();

//		float buff[4];
//		diag_printf("Sampling:\n");
//		cTempMon::get()->getSample(buff);
//		for(int k = 0; k < AN_IN_CNT; k++)
//		{
//			printf("buff[%d]: %.4f\n", k, buff[k]);
//		}
//		cyg_thread_delay(10000);
	}
}


/**
 * This function initiates the whole system and
 * creates the global objects as well
 */
void cInit::init_system()
{
	enable_clocks();
	setup_peripherals();
	create_serial();
	create_io();
	create_spi_devs();

	cLED::init();
	cNVM::init(SECTOR_DATA, SECTOR_STATUS);
	cRTC::init();
	cPwrMon::init();
	cModem::init(SERIAL_SIMCOM_DEVICE);
	cConfig::init(SERIAL_CONFIG_DEVICE);

	cLog::init(SECTOR_LOGS);
	while(!cLog::get());

	cHobbsTimer::init(SECTOR_HOBBS, 6); //set to 6 minutes or 1/10 of an hour
	while(!cHobbsTimer::get());

	cLED::get()->start();
	cTempMon::init();
	cSysMon::init();

	cHobbsTimer::get()->start();

}

void cInit::setup_peripherals()
{
    cyg_uint32 reg32;

    // Make sure SPI3 and UART2 is not remapped. Remap TIM1 to PE. Disable JTAG and SW debug
    HAL_READ_UINT32(CYGHWR_HAL_STM32_AFIO+CYGHWR_HAL_STM32_AFIO_MAPR, reg32);
    reg32 &=~ ( CYGHWR_HAL_STM32_AFIO_MAPR_URT2_RMP |
                CYGHWR_HAL_STM32_AFIO_MAPR_SWJ_MASK |
                CYGHWR_HAL_STM32_AFIO_MAPR_TIM1_FL_RMP);
    reg32 |= CYGHWR_HAL_STM32_AFIO_MAPR_SWJ_SWDPDIS;
    HAL_WRITE_UINT32(CYGHWR_HAL_STM32_AFIO+CYGHWR_HAL_STM32_AFIO_MAPR, reg32);

    // Put SPI2 in SPI mode, and not I2S
    HAL_WRITE_UINT32(CYGHWR_HAL_STM32_SPI2+CYGHWR_HAL_STM32_SPI_I2SCFGR, 0);

    // Put SPI3 in SPI mode, and not I2S
    HAL_WRITE_UINT32(CYGHWR_HAL_STM32_SPI3+CYGHWR_HAL_STM32_SPI_I2SCFGR, 0);

}

void cInit::create_spi_devs()
{
    // Initialize all the SPI devices.
    SpiFlash::init(&stm32_flash_dev);
}

void cInit::create_io()
{
	//setup LED
	setup_pin(LED_R);
	setup_pin(LED_G);

	//setup I2C
	setup_pin(I2C_SCL);
	setup_pin(I2C_SDA);

	//setup Discrete in
	setup_pin(DISC_IN1);
	setup_pin(DISC_IN2);

	//setup Analog in
	setup_pin(ANA_IN1);
	setup_pin(ANA_IN2);
	setup_pin(ANA_IN3);
	setup_pin(ANA_IN4);

	//modem
	setup_pin(SERIAL_SIMCOM_RX);
	setup_pin(SERIAL_SIMCOM_TX);
	set_pinL(SERIAL_SIMCOM_TX);
	setup_pin(SIMCOM_PWR_KEY);
	set_pinL(SIMCOM_PWR_KEY);
	setup_pin(MDM_PWR_OFF);
	set_pinL(MDM_PWR_OFF);

	//config
	setup_pin(SERIAL_CONFIG_RX);
	setup_pin(SERIAL_CONFIG_TX);
}

void cInit::enable_clocks()
{
	// Initialize the external clocks. Perform all resets to get everything in the right state.

	// Here we setup the peripheral clocks. Were a pin is shared by two peripherals, we also make sure that only the one we're going to use is activated.
	cyg_uint32 reg32;
	// Only activate SPI1, UART1, ADC1, IOPA, IOPB, IOPC, IOPD, IOPE and AFIO clocks on APB2
	reg32 = CYGHWR_HAL_STM32_RCC_APB2ENR_SPI1  |
			CYGHWR_HAL_STM32_RCC_APB2ENR_UART1 |
			CYGHWR_HAL_STM32_RCC_APB2ENR_ADC1  |
			CYGHWR_HAL_STM32_RCC_APB2ENR_IOPA  |
			CYGHWR_HAL_STM32_RCC_APB2ENR_IOPB  |
			CYGHWR_HAL_STM32_RCC_APB2ENR_IOPC  |
			CYGHWR_HAL_STM32_RCC_APB2ENR_IOPD  |
			CYGHWR_HAL_STM32_RCC_APB2ENR_IOPE  |
			CYGHWR_HAL_STM32_RCC_APB2ENR_AFIO;
	HAL_WRITE_UINT32(CYGHWR_HAL_STM32_RCC+CYGHWR_HAL_STM32_RCC_APB2ENR, reg32);

	// Only activate SPI3, SPI2, UART2 and UART3 clocks on APB1
	reg32 = //CYGHWR_HAL_STM32_RCC_APB1ENR_SPI3  |
			//CYGHWR_HAL_STM32_RCC_APB1ENR_SPI2  |
			//CYGHWR_HAL_STM32_RCC_APB1ENR_TIM3  |
			CYGHWR_HAL_STM32_RCC_APB1ENR_UART2	|
			//CYGHWR_HAL_STM32_RCC_APB1ENR_UART3	|
			CYGHWR_HAL_STM32_RCC_APB1ENR_UART4	|
    		CYGHWR_HAL_STM32_RCC_APB1ENR_I2C1;
	HAL_WRITE_UINT32(CYGHWR_HAL_STM32_RCC + CYGHWR_HAL_STM32_RCC_APB1ENR, reg32);

	// Disable ETH, USB and embedded flash FLITF on AHB
	HAL_READ_UINT32(CYGHWR_HAL_STM32_RCC+CYGHWR_HAL_STM32_RCC_AHBENR, reg32);
	reg32 &=~ ( CYGHWR_HAL_STM32_RCC_AHBENR_ETHMACRX |
			CYGHWR_HAL_STM32_RCC_AHBENR_ETHMACTX |
			CYGHWR_HAL_STM32_RCC_AHBENR_ETHMAC   |
			CYGHWR_HAL_STM32_RCC_AHBENR_OTGFS    |
			CYGHWR_HAL_STM32_RCC_AHBENR_FLITF );
	HAL_WRITE_UINT32(CYGHWR_HAL_STM32_RCC+CYGHWR_HAL_STM32_RCC_AHBENR, reg32);

}

void cInit::create_serial()
{
}
