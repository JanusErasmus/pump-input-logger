#include <avr/wdt.h>
#include <Arduino.h>
#include <TimerOne.h>

#include "pump_frame.h"
#include "state_logger.h"
#include "event_reporter.h"
#include "led_dui.h"
#include "terminal.h"
#include "pump.h"

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
	wdt_reset();

	Pump.run();

	if(EventReporter.run(&StateLogger))
	{
		wdt_reset();
		delay(1000);
	}
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
