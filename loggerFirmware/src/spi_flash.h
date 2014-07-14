#ifndef _SPI_H
#define _SPI_H

#include <cyg/kernel/kapi.h>
#include <cyg/kernel/diag.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/io/spi.h>
#include <cyg/io/spi_stm32.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"


class SpiFlash : public cDebug
{
private:
   static SpiFlash * __instance;
   cyg_spi_device * Spi_Device;
   SpiFlash(cyg_spi_cortexm_stm32_device_t*);
   bool mReadyFlag;
   cyg_mutex_t busyMutex;

   cyg_uint8 check_status();
   bool flash_verify();
   bool write(cyg_uint32 addr,cyg_uint8 * data_ptr,cyg_uint32 len);

   cyg_uint32 SectSize;
   cyg_uint32 NumSect;
   cyg_uint32 upperAddr;

public:
   static void init(cyg_spi_cortexm_stm32_device_t*);
   static SpiFlash * get();
   bool isReady();

    bool flash_erase(cyg_uint32 addr);
    bool WriteSpi(cyg_uint32 addr,cyg_uint8 * data,cyg_uint32 len);
    bool ReadSpi(cyg_uint32 addr,cyg_uint8 * data,cyg_uint32 len);

    cyg_uint32 GetNumSect();
    cyg_uint32 GetSectSize();
};

#endif
