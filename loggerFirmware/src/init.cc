#include <cyg/kernel/diag.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/kernel/kapi.h>
#include <stdlib.h>

//#include <cyg/io/flash_spi_dev.h> //flash device on SPI
//#include <cyg/io/flash_spi.h> //flash device on SPI

#include "definitions.h"
#include "version.h"
#include "nvm.h"
#include "init.h"
#include "term.h"
#include "led.h"
#include "input_port.h"
#include "MCP_rtc.h"
#include "sys_mon.h"
#include "log.h"
#include "picaxe_lcd.h"

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
	cInit * i = (cInit *)arg;

	i->init_system();

	cyg_thread_delay(50);


	// Initialise the Terminal
	cTerm::init((char *)"/dev/ttydiag",128,"inputLogger>>");


	//Enable alarm interrupts once system has started
	cInput::get()->start();

//	for (;;)
//	{
//
//
//	}
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

	while(!cNVM::get()->isReady())
	{
		diag_printf("Wait nvm...\n");
		cyg_thread_delay(10);
	}

	cyg_uint32 inputPortNumbers[] =
	{
			PORTD_INPUT( 9),
			PORTD_INPUT( 8),
	};
	cInput::init(inputPortNumbers, 2);

	cLED::ledPins_s ledPinNumbers[] = //no pin is 0xFF
	{
			{ PORTE_INPUT(2), PORTE_INPUT(3) },
	};

	cLED::init(ledPinNumbers, 1);

	cRTC::init();

	cSysMon::init();
	cInput::get()->setQueue(cSysMon::get());

	cLog::init(0x40000);

	cPICAXEserialLCD::init(SERIAL_CONFIG_DEVICE);
}

void cInit::setup_peripherals()
{
    cyg_uint32 reg32;

    // Make sure SPI3 and UART2 is not remapped. Remap TIM1 to PE. Disable JTAG and SW debug
    HAL_READ_UINT32(CYGHWR_HAL_STM32_AFIO+CYGHWR_HAL_STM32_AFIO_MAPR, reg32);
    reg32 &=~ ( CYGHWR_HAL_STM32_AFIO_MAPR_SPI3_RMP |
                CYGHWR_HAL_STM32_AFIO_MAPR_URT2_RMP |
                CYGHWR_HAL_STM32_AFIO_MAPR_SWJ_MASK |
                CYGHWR_HAL_STM32_AFIO_MAPR_TIM1_FL_RMP);
    reg32 |= CYGHWR_HAL_STM32_AFIO_MAPR_SWJ_SWDPDIS;
    HAL_WRITE_UINT32(CYGHWR_HAL_STM32_AFIO+CYGHWR_HAL_STM32_AFIO_MAPR, reg32);

    // Put SPI2 in SPI mode, and not I2S
    HAL_WRITE_UINT32(CYGHWR_HAL_STM32_SPI2+CYGHWR_HAL_STM32_SPI_I2SCFGR, 0);

    // Put SPI3 in SPI mode, and not I2S
    HAL_WRITE_UINT32(CYGHWR_HAL_STM32_SPI3+CYGHWR_HAL_STM32_SPI_I2SCFGR, 0);

}

void cInit::create_io()
{
	// Setup the other GPIOs the way we are going to use them.

	//setup I2C
	setup_pin(I2C_SCL);
	setup_pin(I2C_SDA);

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
        reg32 = CYGHWR_HAL_STM32_RCC_APB1ENR_SPI3  |
                CYGHWR_HAL_STM32_RCC_APB1ENR_SPI2  |
                CYGHWR_HAL_STM32_RCC_APB1ENR_TIM3  |
                CYGHWR_HAL_STM32_RCC_APB1ENR_UART2 |
                CYGHWR_HAL_STM32_RCC_APB1ENR_UART3 |
                CYGHWR_HAL_STM32_RCC_APB1ENR_UART4 |
                CYGHWR_HAL_STM32_RCC_APB1ENR_I2C1
                ;
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
