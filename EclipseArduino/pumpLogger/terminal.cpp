#include <HardwareSerial.h>

#include "terminal.h"
#include "event_reporter.h"

Terminal::Terminal()
{

}

void Terminal::handle(String line)
{
	if(line == "stat")
	{
		Serial.print("WiFi status");
		Reporter.printStatus();
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

