#include <WiFi.h>
#include <WiFiClient.h>
#include <avr/wdt.h>
#include <avr/io.h>

#include "reporter.h"
#include "pump_frame.h"
#include "utils.h"

extern PumpFrameClass PumpFrame;

extern "C"
{
	#include <utility/wifi_spi.h>
	//#include <utility/wl_definitions.h>
}

#include "event_reporter.h"
#include "led_dui.h"

extern PumpFrameClass PumpFrame;

EventReporterClass::EventReporterClass()
{
	mState = RP_IDLE;
	mProbed = false;
	mRSSI = -999;
	mLastSync = 0;

	// check for the presence of the shield:
	if (WiFi.status() != WL_NO_SHIELD)
	{
		mProbed = true;
	}
}

//returns true when idle
bool EventReporterClass::run(StateLoggerClass * logger)
{
	if(!mProbed || !logger)
		return false;

	uint8_t state = WiFi.status();
	//printStatus(state);

	switch(mState)
	{
	case RP_UPDATE:
		Serial.println("RP: update");
		{
			if((state == WL_IDLE_STATUS) || (state == WL_CONNECTED))
				mState = RP_CONNECT;
		}
		break;
	case RP_CONNECT:
		Serial.println("RP: connecting");
		switch(state)
		{
		case WL_IDLE_STATUS:
		{
			if((!PumpFrame.ssid[0]) || (!PumpFrame.password[0]))
			{
				Serial.println("Set SSID and password");
				return true;
			}

			wdt_reset();
			int status = WiFi.begin(PumpFrame.ssid, PumpFrame.password);
			if(status != WL_CONNECTED)
			{
				Serial.println("Could not connect");

				resetWiFi();
				return true;
			}
		}
		break;
		case WL_CONNECTED:
		{
			Serial.println("RP: connected");

			IPAddress server(PumpFrame.server);
			wdt_reset();
			if(mClient.connect(server, PumpFrame.port))
			{
				LEDui.setConnecting();
				mState = RP_CLIENT;
			}
			else
			{
				Serial.println("Host unreachable");
				mClient.stop();

				LEDui.setError();
				return true;
			}
		}
		break;

		default:
			return true;
		}
		break;
		case RP_CLIENT:
			Serial.println("RP: wait client");
			if(mClient.connected())
			{
				mState = RP_TRANSFER;
			}

			break;
		case RP_TRANSFER:
			//Serial.print("transfer");

			if(!mClient)
			{
				Serial.println(" NO CLIENT");
				break;
			}

			if(mClient.connected())
			{
				Reporter r(&mClient, logger);
				if(r.transfer())
				{
					mState = RP_DISCONNECT;
					LEDui.setIdle();

					Serial.println("RP: stop client");
					mClient.stop();

					mLastSync = now();
					Serial.println("RP: transfer DONE");
				}
			}
			break;
		case  RP_DISCONNECT:
			Serial.println("RP: disconnect");

			if(mClient.connected())
			{
				mClient.stop();
			}

			if(state == WL_CONNECTED)
			{
				Serial.println("RP: disconnecting");

				resetWiFi();

				mState = RP_IDLE;
			}
			break;

		case RP_IDLE:
			//Serial.println("idle");

			if((timeStatus() != timeSet) || (now() > (mLastSync + PumpFrame.reportRate)))
			{
				Serial.println("RP: Time to report");
				mState = RP_UPDATE;
			}

			return true;
	}

	return false;
}

//setup A5 pin (PINC5) as output and pull low for 500ms, set as floating input again
void EventReporterClass::resetWiFi()
{
	PORTC &= ~(1 << 5); //drive pin low
	DDRC |= (1 << 5); //set PINC5 as output

	delay(500);

	DDRC &= ~(1 << 5); //pin as input again
}

bool EventReporterClass::sync()
{
	if(mState == RP_IDLE)
	{
		mState = RP_UPDATE;
		return true;
	}

	return false;
}

void EventReporterClass::printStatus(int state)
{
	Serial.println("EventReporter State:");
	Serial.print(" - Last transfer: ");
	digitalClockDisplay(mLastSync);
	Serial.print(" - WiFi state: ");

	if(state < 0)
		state = WiFi.status();

		switch(state)
		{
		case WL_NO_SHIELD:
			Serial.println("no shield");
			break;
		case WL_IDLE_STATUS:
			Serial.println("idle");
			break;
		case WL_NO_SSID_AVAIL:
			Serial.println("no SSID");
			break;
		case WL_SCAN_COMPLETED:
			Serial.println("scan complete");
			break;
		case  WL_CONNECTED:
			Serial.println("connected");
			break;
		case WL_CONNECT_FAILED:
			Serial.println("connect failed");
			break;
		case WL_CONNECTION_LOST:
			Serial.println("connect lost");
			break;
		case WL_DISCONNECTED:
			Serial.println("disconnected");
			break;
		}
}

uint8_t EventReporterClass::status()
{
	return WiFi.status();
}

EventReporterClass::~EventReporterClass()
{
}


EventReporterClass EventReporter;

