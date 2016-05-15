#ifndef WIFI_CONNECTOR_H_
#define WIFI_CONNECTOR_H_
#include <IPAddress.h>
#include <WiFiClient.h>

#include "event_logger.h"
#include "Time.h"

class WiFiConnectorClass
{
public:
	enum eReportState
	{
		RP_UPDATE,
		RP_CONNECT,
		RP_CLIENT,
		RP_TRANSFER,
		RP_DISCONNECT,
		RP_IDLE
	};

private:

	time_t mLastSync;
	eReportState mState;
	WiFiClient mClient;
	int mRSSI;
	bool mProbed;


public:
	WiFiConnectorClass();
	virtual ~WiFiConnectorClass();

	bool run(EventLoggerClass * logger);
	bool sync();
	void resetWiFi();

	uint8_t status();
	void printStatus(int status = -1);
};

extern WiFiConnectorClass WiFiConnector;

#endif /* WIFI_CONNECTOR_H_ */
