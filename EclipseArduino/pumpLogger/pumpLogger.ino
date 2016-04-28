#include <avr/wdt.h>
#include <Arduino.h>
#include <TimerOne.h>

#include "pump_frame.h"
#include "state_logger.h"
#include "event_reporter.h"
#include "led_dui.h"
#include "terminal.h"

PumpFrameClass PumpFrame(0);
StateLoggerClass StateLogger(32);

void tenthSecond(void)
{
	LEDui.run();
}

void setup()
{
	wdt_enable(WDTO_8S);

	//Initialize serial and wait for port to open:
	Serial.begin(9600);
	Serial.println("Wifi Pump Logger");

	LEDui.init(6); //green wire, TOP LED

	pinMode(3, INPUT_PULLUP); //PUMP blue
	pinMode(5, OUTPUT); //white wire, relay
	pinMode(9, OUTPUT); //LED on WiFi SHIELD

	digitalWrite(9, LOW);
	digitalWrite(5, LOW);

	Timer1.initialize(100000);
	Timer1.attachInterrupt(tenthSecond);


}

void loop()
{
	wdt_reset();

	EventReporter.service();

	wdt_reset();

	delay(1000);
}

void serialEvent()
{
	static String inputString;

	while (Serial.available())
	{
		// get the new byte:
		char inChar = (char)Serial.read();

		// add it to the inputString:
		inputString += inChar;

		if (inChar == '\n' || inChar == '\r')
		{
			Terminal::handle(inputString);
			inputString = "";
		}
	}
}


uint8_t calc_crc(uint8_t * buff, uint8_t len)
{
	uint8_t csum = 0;
	for(uint8_t k = 0; k < len; k++)
		csum ^= buff[k];

	return csum;
}
