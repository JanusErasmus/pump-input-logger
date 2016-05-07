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

	if(line == "sync")
	{
		if(WiFiConnector.sync())
			Serial.println("Sync scheduled");
		else
			Serial.println("Reporter is busy");

		return;
	}

	if(line == "log")
	{
		Event evt;
		EventLogger.log(&evt);
		evt.print();
		return;
	}

	if(line == "stat")
	{
		WiFiConnector.printStatus();
		return;
	}

	if(line == "time")
	{
		Serial.print("Time: ");

		switch(timeStatus())
		{
		case timeSet:
			digitalClockDisplay();
			break;

		case timeNotSet:
			Serial.println(" not set");
			break;

		case timeNeedsSync:
			Serial.println(" need sync");
			break;
		}

		return;
	}

	if(line == "cfg")
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

		if(cmd == "setip")
		{
			Serial.print("Set IP: "); Serial.println(arg);

			PumpFrame.setIP(arg, 0);
		}
		else if(cmd == "setport")
		{
			Serial.print("Set PORT: "); Serial.println(arg);

			PumpFrame.setPort(arg ,0);
		}

		if(cmd == "setid")
		{
			Serial.print("Set ID: "); Serial.println(arg);

			PumpFrame.setId(arg, 0);
		}

		if(cmd == "setpass")
		{
			Serial.print("Set Pass: "); Serial.println(arg);

			PumpFrame.setPass(arg, 0);
		}
	}
}

Terminal::~Terminal()
{
}

