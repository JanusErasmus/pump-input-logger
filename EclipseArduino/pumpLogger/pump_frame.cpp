#include "pump_frame.h"

#include <EEPROM.h>

#include "state_logger.h"

extern uint8_t calc_crc(uint8_t * buff, uint8_t len);

PumpFrameClass::PumpFrameClass()
{  
    reportRate = 300;
    startHour = 0xFF;
    endHour = 0xFF;
    upTime = 0;
    restTime = 0;
    port = 60000;
    server[0] = 192;
    server[1] = 168;
    server[2] = 1;
    server[3] = 242;
    crc = 0xFF;
}

PumpFrameClass::PumpFrameClass(int address)
{  
	reportRate = 300;

	EEPROM.get(address, *this);

	if(calc_crc((uint8_t*)this, sizeof(PumpFrameClass)))
	{
		startHour = 0xFF;
		endHour = 0xFF;
		upTime = 0;
		restTime = 0;
		port = 60000;
		server[0] = 192;
		server[1] = 168;
		server[2] = 1;
		server[3] = 242;
		crc = 0xFF;
	}
}

void PumpFrameClass::store(int address)
{  
  EEPROM.put(address, *this);
}

boolean PumpFrameClass::equals(PumpFrameClass * frame)
{
  return !memcmp(this, frame, sizeof(PumpFrameClass));
}

void PumpFrameClass::setIP(String ipString, int flashAddress)
{
  uint8_t ip[5];

  memset(ip, 0,4);

  int index = 0;
   int subIndex = ipString.indexOf('.');
   do
   {
     String arg = ipString.substring(0, subIndex);
     ipString = ipString.substring(subIndex + 1);
     Serial.println(arg);
      ip[index++] = arg.toInt();
     
     subIndex = ipString.indexOf('.');   
     
      ip[index + 1] = ipString.substring(subIndex + 1).toInt();
   }while((subIndex > 0) && (index < 3));

  if(index > 2)
   {
    Serial.println("Found valid ip");
    memcpy(server, ip, 4);

    crc = calc_crc((uint8_t*)this, (sizeof(PumpFrameClass) - 1));

    store(flashAddress);

    print();
   }
   
}

void PumpFrameClass::setPort(String portString, int flashAddress)
{
    port = portString.toInt();

    crc = calc_crc((uint8_t*)this, (sizeof(PumpFrameClass) - 1));

    store(flashAddress);

    print();
}

void PumpFrameClass::print()
{
  Serial.println("PumpFrame:");
  Serial.print(" rate : "); Serial.println(reportRate);
  Serial.print(" start: "); Serial.println(startHour);
  Serial.print(" end  : "); Serial.println(endHour);
  Serial.print(" up   : "); Serial.println(upTime);
  Serial.print(" rest : "); Serial.println(restTime);
  Serial.print(" srv  : "); Serial.print(server[0]); Serial.print("."); Serial.print(server[1]); Serial.print("."); Serial.print(server[2]); Serial.print("."); Serial.println(server[3]);
  Serial.print(" port : "); Serial.println(port);
  Serial.print(" crc  : "); Serial.println(crc);
}

PumpFrameClass& PumpFrameClass::operator=(PumpFrameClass &frame)
{
  memcpy(this, &frame, sizeof(PumpFrameClass));

  return *this;
}
