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
    cl_brate      : 8000000,    // baud rate in Hz
    cs_up_udly    : 0,         // delay in usec between CS up and transfer start
    cs_dw_udly    : 0,         // delay in usec between transfer end and CS down
    tr_bt_udly    : 0          // delay in usec between two transfers
};

