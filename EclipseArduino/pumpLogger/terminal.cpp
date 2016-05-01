#include <Arduino.h>

#include "terminal.h"
#include "event_reporter.h"
#include "utils.h"

extern StateLoggerClass StateLogger;
extern EventReporterClass EventReporter;

Terminal::Terminal()
{
}

void Terminal::handle(String line)
{
	line.trim();
	//Serial.println(line);

	if(line == "sync")
	{
		if(EventReporter.sync())
			Serial.println("Sync scheduled");
		else
			Serial.println("Reporter is busy");

		return;
	}

	if(line == "log")
	{
		Event evt;
		StateLogger.log(&evt);
		evt.print();
		return;
	}

	if(line == "stat")
	{
		EventReporter.printStatus();
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

  int subIndex = line.indexOf(' ');
  if(subIndex > 0)
  {
    String cmd = line.substring(0, subIndex);
    String arg = line.substring(subIndex + 1);

    //Serial.println(cmd);
    //Serial.println(arg);

//    if(cmd == "setip")
//    {
//      Serial.print("Set IP: "); Serial.println(arg);
//
//      pumpFrame.setIP(arg, 0);
//
//      //pumpFrame.crc = calc_crc((uint8_t*)&pumpFrame, (sizeof(PumpFrame) - 1));
//    }
//    else if(cmd == "setport")
//    {
//      Serial.print("Set PORT: "); Serial.println(arg);
//
//      pumpFrame.setPort(arg ,0);
//    }
//    else if(cmd == "cfg")
//    {
//      pumpFrame.print();
//    }
  }
}

Terminal::~Terminal()
{
}

