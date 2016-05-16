#include <WiFi.h>
#include <WiFiClient.h>
#include <avr/wdt.h>
#include <avr/io.h>

#include "event_reporter.h"
#include "pump_frame.h"
#include "utils.h"

extern PumpFrameClass PumpFrame;

extern "C"
{
	#include <utility/wifi_spi.h>
	//#include <utility/wl_definitions.h>
}

#include "wifi_connector.h"
#include "led_ui.h"

extern PumpFrameClass PumpFrame;

WiFiConnectorClass::WiFiConnectorClass()
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
bool WiFiConnectorClass::run(EventLoggerClass * logger)
{
	if(!mProbed || !logger)
	{
		resetWiFi();
		return true;
	}

	uint8_t state = WiFi.status();
	//printStatus(state);

	switch(mState)
	{
	case RP_UPDATE:
		Serial.println(F("RP: update"));
		{
			if((state == WL_IDLE_STATUS) || (state == WL_CONNECTED))
				mState = RP_CONNECT;
		}
		break;
	case RP_CONNECT:
		Serial.println(F("RP: connect"));
		switch(state)
		{
		case WL_IDLE_STATUS:
		{
			Serial.println(F("WL: idle"));
			if((!PumpFrame.ssid[0]) || (!PumpFrame.password[0]))
			{
				Serial.println(F("Set SSID and password"));
				return true;
			}

			LEDui.setError();


			wdt_reset();
			int status = WiFi.begin(PumpFrame.ssid, PumpFrame.password);
			if(status != WL_CONNECTED)
			{
				Serial.println(F("Could not connect"));

				resetWiFi();
				return true;
			}

			LEDui.setWifi();
		}
		break;
		case WL_CONNECTED:
		{
			Serial.println(F("WL: connected"));

			IPAddress server(PumpFrame.server);
			wdt_reset();
			if(mClient.connect(server, PumpFrame.port))
			{
				LEDui.setConnecting();
				mState = RP_CLIENT;
			}
			else
			{
				Serial.println(F("Host unreachable"));
				mClient.stop();

				return true;
			}
		}
		break;

		default:
			return true;
		}
		break;
		case RP_CLIENT:
			Serial.println(F("RP: wait client"));
			if(mClient.connected())
			{
				mState = RP_TRANSFER;
			}

			break;
		case RP_TRANSFER:
			//Serial.print(F("RP: transfer"));

			if(!mClient)
			{
				Serial.println(F(" NO CLIENT"));
				break;
			}

			if(mClient.connected())
			{
				EventReporter r(&mClient, logger);
				if(r.transfer())
				{
					mState = RP_DISCONNECT;
					LEDui.setIdle();

					Serial.println(F("RP: stop client"));
					mClient.stop();

					mLastSync = now();
					Serial.println(F("RP: transfer DONE"));
				}
			}
			break;
		case  RP_DISCONNECT:
			Serial.println(F("RP: disconnect"));

			if(mClient.connected())
			{
				mClient.stop();
			}

			if(state == WL_CONNECTED)
			{
				Serial.println(F("RP: disconnecting"));

				resetWiFi();

				mState = RP_IDLE;
			}
			break;

		case RP_IDLE:
			//Serial.println(F("idle"));

			if((timeStatus() != timeSet) || (now() > (mLastSync + PumpFrame.reportRate)))
			{
				Serial.println(F("RP: Time to report"));
				mState = RP_UPDATE;
			}

			return true;
	}

	return false;
}

//setup A5 pin (PINC5) as output and pull low for 500ms, set as floating input again
void WiFiConnectorClass::resetWiFi()
{
	PORTC &= ~(1 << 5); //drive pin low
	DDRC |= (1 << 5); //set PINC5 as output

	delay(500);

	DDRC &= ~(1 << 5); //pin as input again
}

bool WiFiConnectorClass::sync()
{
	if(mState == RP_IDLE)
	{
		mState = RP_UPDATE;
		return true;
	}

	return false;
}

void WiFiConnectorClass::printStatus(int state)
{
	Serial.println(F("EventReporter State:"));
	Serial.print(F(" - Last transfer: "));
	digitalClockDisplay(mLastSync);
	Serial.print(F(" - WiFi state: "));

	if(state < 0)
		state = WiFi.status();

		switch(state)
		{
		case WL_NO_SHIELD:
			Serial.println(F("no shield"));
			break;
		case WL_IDLE_STATUS:
			Serial.println(F("idle"));
			break;
		case WL_NO_SSID_AVAIL:
			Serial.println(F("no SSID"));
			break;
		case WL_SCAN_COMPLETED:
			Serial.println(F("scan complete"));
			break;
		case  WL_CONNECTED:
			Serial.println(F("connected"));
			break;
		case WL_CONNECT_FAILED:
			Serial.println(F("connect failed"));
			break;
		case WL_CONNECTION_LOST:
			Serial.println(F("connect lost"));
			break;
		case WL_DISCONNECTED:
			Serial.println(F("disconnected"));
			break;
		}
}

uint8_t WiFiConnectorClass::status()
{
	return WiFi.status();
}

WiFiConnectorClass::~WiFiConnectorClass()
{
}


WiFiConnectorClass WiFiConnector;

