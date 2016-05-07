#include <avr/wdt.h>
#include <Arduino.h>
#include <TimerOne.h>

#include "event_logger.h"
#include "pump_frame.h"
#include "led_dui.h"
#include "terminal.h"
#include "pump.h"
#include "wifi_connector.h"

PumpFrameClass PumpFrame(0);
EventLoggerClass EventLogger(128);

void tenthSecond(void)
{
	LEDui.run();
}

void setup()
{
	wdt_enable(WDTO_8S);

	//Initialize serial and wait for port to open:
	Serial.begin(9600);
	Serial.println(F("\n\nWifi Pump Logger"));

	LEDui.init(6); //green wire, TOP LED
	Pump.init(5, 3); //pin 5: white wire, relay; //pin 3: PUMP blue

	pinMode(9, OUTPUT); //LED on WiFi SHIELD

	digitalWrite(9, LOW);
	digitalWrite(5, LOW);

	Timer1.initialize(100000);
	Timer1.attachInterrupt(tenthSecond);

	PumpFrame.print();

	setSyncInterval(86400);
}

void loop()
{
	Pump.run();

	if(WiFiConnector.run(&EventLogger))
	{
		delay(1000);
	}

	wdt_reset();
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
		//Serial.println(inputString);

		if (inChar == '\n' || inChar == '\r')
		{
			Terminal::handle(inputString);
			inputString = "";
		}
	}
}
