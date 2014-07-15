#include <cyg/io/at25dfxxx.h>

#include "spi_dev.h"

cyg_spi_cortexm_stm32_device_t stm32_flash_dev CYG_SPI_DEVICE_ON_BUS(1)=
{
    spi_device :
    {
        spi_bus:  &cyg_spi_stm32_bus1.spi_bus
    },

    dev_num       : 0,         // number of the device. in other words, the position of the specific chip select GPIO in the CYGHWR_DEVS_SPI_CORTEXM_STM32_BUSx_CS_GPIOS parameter.
    bus_16bit     : false,     // true if the device uses 16 bit transactions and false if the bus uses 8 bit transactions.
    cl_pol        : 0,         // clock polarity. must be 1 if a clock line pullup resistor is used and 0 if a clock line pulldown resistor is used.
    cl_pha        : 0,         // clock phase
    cl_brate      : 5000000,    // baud rate in Hz
    cs_up_udly    : 100,         // delay in usec between CS up and transfer start
    cs_dw_udly    : 100,         // delay in usec between transfer end and CS down
    tr_bt_udly    : 1000          // delay in usec between two transfers
};



struct cyg_flash_block_info dataflash0_block_info;
static struct cyg_flash_dev dataflash0 CYG_HAL_TABLE_ENTRY(cyg_flashdev) =
{
		funs : &cyg_devs_flash_spi_at25dfxxx_funs,
		flags : 0,
		start : 0,
		end : 0x80000,
		num_block_infos : 1,
		block_info : &dataflash0_block_info,
		priv : (const void*) &(stm32_flash_dev.spi_device)
};


void spi_dev_global_unprotect()
{
	cyg_spi_device* dev = &stm32_flash_dev.spi_device;
	cyg_uint8 tx_buff[4];

	tx_buff[0] = 0x06;
	cyg_spi_transfer(dev,false,1,tx_buff,NULL);

	tx_buff[0] = 0x01;
	tx_buff[1] = 0x00;
	cyg_spi_transfer(dev,false,2,tx_buff,NULL);
}
