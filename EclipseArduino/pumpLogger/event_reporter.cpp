#include "event_reporter.h"

#include <WiFi.h>
#include <avr/wdt.h>

#include "pump_frame.h"
#include "Time.h"
#include "utils.h"

extern PumpFrameClass PumpFrame;

EventReporter::EventReporter(WiFiClient * client, EventLoggerClass * logger) : mClient(client), mLogger(logger)
{
}

bool EventReporter::transfer()
{
	if(!mClient)
		return true;

	Serial.println("Client transferring");

	byte mac[6];
	WiFi.macAddress(mac);
	String macStr = getMACstring(mac);

	mClient->flush();
	mClient->print("RSSI: ");
	mClient->println(WiFi.RSSI());
	mClient->println(macStr);

	String statusStr;
	statusStr += getStateString(3);
	statusStr += getStateString(5);
	statusStr += getStateString(6);
	wdt_reset();
	mClient->println(statusStr);

	replyLogs();

	bool txStatus = false;
	do
	{
		txStatus = serviceServerData();
		if(txStatus)
			break;

//	    //wait 10s for reply from server
//	    DateTime.available();
//	    if(DateTime.now() > (startTransfer + 10))
//	    {
//	      Serial.println("time Out");
//	      break;
//	    }
	}while(!txStatus);

	return true;
}

void EventReporter::replyLogs()
{
	mLogger->reset();

	while(1)
	{
		wdt_reset();
		Event evt = mLogger->getEvent();
		if(!evt.crc)
			break;

		Serial.print("Got evt: ");
		digitalClockDisplay(evt.timeStamp);

		String log = evt.getString();
		mClient->println(log);
	}
}

bool EventReporter::serviceServerData()
{
  static String rxString;
  boolean ackFlag = false;
  static PumpFrameClass frame;

  while (mClient->available())
  {
    char c = mClient->read();
    //Serial.write(c);

    rxString += String(c);

    if(rxString.endsWith("\n") || rxString.endsWith("\r"))
    {
      rxString.trim();

      if(rxString.length() < 2)
        continue;

      //Serial.print("RX: ");
      //Serial.println(rxString);

      if(rxString.charAt(0) == 'T')
      {
        long timeStamp = rxString.substring(1).toInt();
        setTime(timeStamp);

        Serial.print("Time: ");
        digitalClockDisplay(timeStamp);

        //keep server settings
        memcpy(frame.server, PumpFrame.server, 68);
        frame.port = PumpFrame.port;

        frame.crc = calc_crc((uint8_t*)&frame, (sizeof(PumpFrame) - 1));
        if(!PumpFrame.equals(&frame))
        {
          PumpFrame = frame;
          PumpFrame.store(0);

          Serial.println("Update pump frame values");
          PumpFrame.print();
        }

        ackFlag = true;
      }

      if(rxString.charAt(0) == 'A')
      {
        mLogger->reset();

        int ack = rxString.substring(1).toInt();
        Serial.print("Ack: ");
        Serial.println(ack);

        for(uint8_t k = 0; k < ack; k++)
        {
          mLogger->ack();
        }
      }

      if(rxString.charAt(0) == 'F')
      {
        if(rxString.charAt(1) == 'p')
        {
           int value = rxString.substring(2).toInt();
           frame.reportRate = value;
        }
        if(rxString.charAt(1) == 's')
        {
           int value = rxString.substring(2).toInt();
           //Serial.print("start: ");
           //Serial.println(value);
           frame.startHour = value;
        }
        if(rxString.charAt(1) == 'e')
        {
           int value = rxString.substring(2).toInt();
           //Serial.print("end : ");
           //Serial.println(value);
           frame.endHour = value;
        }
        if(rxString.charAt(1) == 'u')
        {
           int value = rxString.substring(2).toInt();
           //Serial.print("up : ");
           //Serial.println(value);
           frame.upTime = value;
        }
        if(rxString.charAt(1) == 'r')
        {
           int value = rxString.substring(2).toInt();
           //Serial.print("rest : ");
           //Serial.println(value);
           frame.restTime = value;
        }
      }

      rxString = String();
    }
  }

  return ackFlag;
}

String EventReporter::getMACstring(byte mac[6])
{
  String macString;

  macString += String(mac[5],HEX);
  macString += String(":");
  macString += String(mac[4],HEX);
  macString += String(":");
  macString += String(mac[3],HEX);
  macString += String(":");
  macString += String(mac[2],HEX);
  macString += String(":");
  macString += String(mac[1],HEX);
  macString += String(":");
  macString += String(mac[0],HEX);

  return macString;
}

String EventReporter::getStateString(int port)
{
  String str;

  if(digitalRead(port))
  {
    str += String("S");
  }
  else
  {
    str += String("R");
  }

  str += String(port);
  str += String("\r\n");

  return str;
}

EventReporter::~EventReporter()
{

}

