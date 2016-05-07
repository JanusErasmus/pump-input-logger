#include <Arduino.h>

#include "terminal.h"
#include "utils.h"
#include "pump_frame.h"
#include "wifi_connector.h"

extern EventLoggerClass EventLogger;
extern WiFiConnectorClass WiFiConnector;
extern PumpFrameClass PumpFrame;

Terminal::Terminal()
{
}

void Terminal::handle(String line)
{
	line.trim();
	//Serial.println(line);

	if(line == F("sync"))
	{
		if(WiFiConnector.sync())
			Serial.println(F("Sync scheduled"));
		else
			Serial.println(F("Reporter is busy"));

		return;
	}

	if(line == F("log"))
	{
		Event evt;
		EventLogger.log(&evt);
		evt.print();
		return;
	}

	if(line == F("stat"))
	{
		WiFiConnector.printStatus();
		return;
	}

	if(line == F("time"))
	{
		Serial.print(F("Time: "));

		switch(timeStatus())
		{
		case timeSet:
			digitalClockDisplay();
			break;

		case timeNotSet:
			Serial.println(F(" not set"));
			break;

		case timeNeedsSync:
			Serial.println(F(" need sync"));
			break;
		}

		return;
	}

	if(line == F("cfg"))
	{
		PumpFrame.print();

		return;
	}

	int subIndex = line.indexOf(' ');
	if(subIndex > 0)
	{
		String cmd = line.substring(0, subIndex);
		String arg = line.substring(subIndex + 1);

		//Serial.println(cmd);
		//Serial.println(arg);

		if(cmd == F("setip"))
		{
			Serial.print(F("Set IP: ")); Serial.println(arg);

			PumpFrame.setIP(arg, 0);
		}
		else if(cmd == F("setport"))
		{
			Serial.print(F("Set PORT: ")); Serial.println(arg);

			PumpFrame.setPort(arg ,0);
		}

		if(cmd == F("setid"))
		{
			Serial.print(F("Set ID: ")); Serial.println(arg);

			PumpFrame.setId(arg, 0);
		}

		if(cmd == F("setpass"))
		{
			Serial.print(F("Set Pass: ")); Serial.println(arg);

			PumpFrame.setPass(arg, 0);
		}
	}
}

Terminal::~Terminal()
{
}

