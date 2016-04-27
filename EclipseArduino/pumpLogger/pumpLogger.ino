#include <avr/wdt.h>
#include <Arduino.h>
#include <TimerOne.h>

#include "event_reporter.h"
#include "terminal.h"
#include "led_ui.h"

void tenthSecond(void)
{
	LED.run();
}

//The setup function is called once at startup of the sketch
void setup()
{
	wdt_enable(WDTO_8S);

	//Initialize serial and wait for port to open:
	Serial.begin(9600);
	Serial.println("Wifi Pump Logger");

	LED.init(6); //green wire, TOP LED

	pinMode(3, INPUT_PULLUP); //PUMP blue
	pinMode(5, OUTPUT); //white wire, relay
	pinMode(9, OUTPUT); //LED on WiFi SHIELD

	digitalWrite(9, LOW);
	digitalWrite(5, LOW);

	Timer1.initialize(100000);
	Timer1.attachInterrupt(tenthSecond);


}

// The loop function is called in an endless loop
void loop()
{

	wdt_reset();

	Reporter.service();

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
