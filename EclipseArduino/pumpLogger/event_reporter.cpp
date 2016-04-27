#include <WiFi.h>
#include <utility/wl_definitions.h>
#include <utility/wifi_spi.h>

#include "event_reporter.h"
#include "led_ui.h"

EventReporter::EventReporter(const char * ssid, const char * pass, IPAddress server)
{
	mState = RP_UNKNOWN;
	mProbed = false;
	mRSSI = -999;
	mSSID = ssid;
	mPassword = pass;
	mServer = server;
	mClient = 0;

	// check for the presence of the shield:
	if (WiFi.status() != WL_NO_SHIELD)
	{
		mProbed = true;
	}

}

void EventReporter::service()
{
	if(!mProbed)
		return;

	uint8_t state = WiFi.status();

	printStatus(state);

	switch(mState)
	{
	case RP_UNKNOWN:
		Serial.println("unknown");
		{
			if((state == WL_IDLE_STATUS) || (state == WL_CONNECTED))
				mState = RP_CONNECT;
		}
		break;
	case RP_IDLE:
		Serial.println("idle");
		break;
	case RP_CONNECT:
		Serial.println("connect");
		switch(state)
		{
		case WL_IDLE_STATUS:
		{
			WiFi.begin((char*)mSSID, (char*)mPassword);
		}
		break;
		case WL_CONNECTED:
		{
			Serial.println("connected");
			LED.setConnecting();

			if(!mClient)
				mClient = new WiFiClient();

			if(mClient->connect(mServer, 60000))
			{
				mState = RP_CLIENT;
			}
			else
			{
				Serial.println("Host unreachable");
				mClient->stop();
				delete mClient;
				mClient = 0;
			}

		}
		break;

		default:
			break;
		}
		break;
		case RP_CLIENT:
			Serial.println("client");
			if(mClient->connected())
			{
				mState = RP_TRANSFER;
			}

			break;
		case RP_TRANSFER:
			Serial.print("transfer");

			if(!mClient)
			{
				Serial.println(" NO CLIENT");
				break;
			}

			Serial.println(mClient->status());

			if(mClient->connected())
			{
				static uint8_t count = 5;
				Serial.print("transferring");
				Serial.println(count);

				if(count-- == 0)
				{
					mState = RP_IDLE;


					LED.setIdle();
				}

				//mClient.write("hello\r\n");
			}
			break;
		case  RP_DISCONNECT:
			Serial.println("disconnect");


			if(mClient && mClient->connected())
			{
				Serial.println("stop client");
				mClient->stop();
				delete mClient;
				mClient = 0;
			}

			if(state == WL_CONNECTED)
			{
				Serial.println("disconnecting");
				WiFi.disconnect();

				mState = RP_IDLE;
			}
			break;
	}

}

void EventReporter::printStatus(int state)
{
	Serial.print("WiFi state: ");

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

uint8_t EventReporter::status()
{
	return WiFi.status();
}

EventReporter::~EventReporter()
{
	if(mClient)
		delete mClient;

	Serial.println("EventReporter: deleted");
}


EventReporter Reporter("J_C", "VictorHanny874", IPAddress(192, 168, 1, 242));

