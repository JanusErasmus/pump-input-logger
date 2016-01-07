#include <EEPROM.h>
#include <StateLogger.h> //calc_crc used from StateLogger

#include "PumpFrame.h"


PumpFrame::PumpFrame()
{  
    startHour = 0xFF;
    endHour = 0xFF;
    upTime = 0;
    restTime = 0;
    crc = 0xFF;
}

PumpFrame::PumpFrame(int address)
{  
  EEPROM.get(address, *this);

  if(calc_crc((uint8_t*)this, sizeof(PumpFrame)))
  {
    startHour = 0xFF;
    endHour = 0xFF;
    upTime = 0;
    restTime = 0;
    crc = 0xFF;
  }
}

void PumpFrame::store(int address)
{  
  EEPROM.put(address, *this);
}

boolean PumpFrame::equals(PumpFrame * frame)
{
  return !memcmp(this, frame, sizeof(PumpFrame));
}

void PumpFrame::print()
{
  Serial.println("PumpFrame:");
  Serial.print(" start: ");
  Serial.println(startHour);
  Serial.print(" end  : ");
  Serial.println(endHour);
  Serial.print(" up   : ");
  Serial.println(upTime);
  Serial.print(" rest : ");
  Serial.println(restTime);
  Serial.print(" crc  : ");
  Serial.println(crc);
}

PumpFrame& PumpFrame::operator=(PumpFrame &frame)
{
  memcpy(this, &frame, sizeof(PumpFrame));
}