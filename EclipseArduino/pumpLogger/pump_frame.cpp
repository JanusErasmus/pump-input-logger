#include "pump_frame.h"

#include <EEPROM.h>

#include "event_logger.h"

extern uint8_t calc_crc(uint8_t * buff, uint8_t len);

PumpFrameClass::PumpFrameClass()
{  
    reportRate = 300;
    startHour = 0xFF;
    endHour = 0xFF;
    upTime = 0;
    restTime = 0;
    port = 0;
    memset(server, 0, 68);
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
		port = 0;
		memset(server, 0, 68);
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
		Serial.println(F("Found valid ip"));
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

void PumpFrameClass::setId(String addrString, int flashAddress)
{
    addrString.toCharArray(ssid, 32);

    crc = calc_crc((uint8_t*)this, (sizeof(PumpFrameClass) - 1));

    store(flashAddress);

    print();
}

void PumpFrameClass::setPass(String addrString, int flashAddress)
{
    addrString.toCharArray(password, 31);

    crc = calc_crc((uint8_t*)this, (sizeof(PumpFrameClass) - 1));

    store(flashAddress);

    print();
}

void PumpFrameClass::print()
{
  Serial.println(F("PumpFrame:"));
  Serial.print(F(" rate : ")); Serial.println(reportRate);
  Serial.print(F(" start: ")); Serial.println(startHour);
  Serial.print(F(" end  : ")); Serial.println(endHour);
  Serial.print(F(" up   : ")); Serial.println(upTime);
  Serial.print(F(" rest : ")); Serial.println(restTime);
  Serial.print(F(" srv  : ")); Serial.print(server[0]); Serial.print("."); Serial.print(server[1]); Serial.print("."); Serial.print(server[2]); Serial.print("."); Serial.println(server[3]);
  Serial.print(F(" port : ")); Serial.println(port);
  Serial.print(F(" ssid : ")); Serial.println(ssid);
  Serial.print(F(" Pass : ")); Serial.println(password);
  Serial.print(F(" crc  : ")); Serial.println(crc);
}

PumpFrameClass& PumpFrameClass::operator=(PumpFrameClass &frame)
{
  memcpy(this, &frame, sizeof(PumpFrameClass));

  return *this;
}
