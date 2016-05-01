#include <Arduino.h>
#include "Time.h"

#include "utils.h"

uint8_t calc_crc(uint8_t * buff, uint8_t len)
{
	uint8_t csum = 0;
	for(uint8_t k = 0; k < len; k++)
		csum ^= buff[k];

	return csum;
}

void digitalClockDisplay()
{
  // digital clock display of current time
	digitalClockDisplay(now());
}

void digitalClockDisplay(time_t timeStamp)
{

	tmElements_t tm;
	breakTime(timeStamp, tm);

   digitalClockDisplay(tm);
}

void digitalClockDisplay(tmElements_t tm)
{
  Serial.print(tm.Hour, DEC);
  printDigits(tm.Minute);
  printDigits(tm.Second);
  Serial.print(" ");
  Serial.print(dayStr(tm.Wday));
  Serial.print(" ");
  Serial.print(tm.Day, DEC);
  Serial.print(" ");
  Serial.print(monthStr(tm.Month));
  Serial.print(" ");
  Serial.println(1970 + tm.Year);
}

void printDigits(byte digits)
{
  // utility function for digital clock display: prints colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits,DEC);
}
