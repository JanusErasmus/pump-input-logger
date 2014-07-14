#ifndef _SPI_DEV_H_
#define _SPI_DEV_H_

#include <cyg/io/spi.h>
#include <cyg/io/spi_stm32.h>

void spi_dev_global_unprotect();

extern cyg_spi_cortexm_stm32_device_t stm32_flash_dev;

#endif




