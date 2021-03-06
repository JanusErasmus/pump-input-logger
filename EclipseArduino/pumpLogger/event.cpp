#include <EEPROM.h>

#include "event.h"

#include "utils.h"

Event::Event()
{
  timeStamp = now();
  port = 3;
  state = 0;
  ack = 0xFF;
  crc = calc_crc((uint8_t*)this, sizeof(Event) - 1);
}

Event::Event(time_t timeStamp, uint8_t port, uint8_t state)
{
  this->timeStamp = timeStamp;
  this->port = port;
  this->state = state;
  ack = 0xFF;

  crc = calc_crc((uint8_t*)this, sizeof(Event) - 1);
}

Event::Event(int address)
{
  Event e;
  EEPROM.get(address, e);
  memcpy(this, &e, sizeof(Event));

  if(!isValid())
  {
      //Serial.print("INVALID EVENT!");
      timeStamp = 0;
      port = 0;
      state = 0;
      ack = 0xFF;
      crc = 0;
  }
}
void Event::acknowledge()
{
	ack = 0;
	crc = calc_crc((uint8_t*)this, sizeof(Event) - 1);
}

boolean Event::isValid()
{
  boolean blank = true;
  uint8_t * p = (uint8_t*)this;

	for(uint8_t k =0; k < sizeof(Event); k++)
	{
		if(p[k] != 0xFF)
		{
			blank = false;
			break;
		}
	}

	if(blank)
		return 0;

	if(calc_crc((uint8_t*)this, sizeof(Event)))
	{
		Serial.println(F("Event: INVALID CRC"));
		return 0;
	}

  return 1;
}

void Event::store(int address)
{
  EEPROM.put(address, *this);
}

String Event::getString()
{
  String txt(timeStamp);
  txt += String(",");
  txt += String(port);
  txt += String(",");
  txt += String(state);

  return txt;
}

 void Event::print()
{
  if(!crc)
  {
    Serial.println(F("Invalid event"));
    return;
  }

  Serial.println(F("Event"));
  Serial.print(" time: ");
  digitalClockDisplay(timeStamp);
  Serial.print(F(" Port: "));
  Serial.println(port);
  Serial.print(F(" Stat: "));
  Serial.println(state);
  Serial.print(F(" ACK : "));
  Serial.println(ack);
  Serial.print(F(" CRC : "));
  Serial.println(crc);
 }


