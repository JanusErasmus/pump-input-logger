#ifndef EVENT_REPORTER_H_
#define EVENT_REPORTER_H_
#include <IPAddress.h>
#include <WiFiClient.h>

#include "state_logger.h"
#include "Time.h"

class EventReporterClass
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
	bool mProbed;

	time_t mLastSync;
	eReportState mState;

	int mRSSI;
	const char * mSSID;
	const char * mPassword;
	IPAddress mServer;
	WiFiClient mClient;

public:
	EventReporterClass(const char * ssid, const char * pass, IPAddress server);
	virtual ~EventReporterClass();

	bool run(StateLoggerClass * logger);
	bool sync();

	uint8_t status();
	void printStatus(int status = -1);
};

extern EventReporterClass EventReporter;

#endif /* EVENT_REPORTER_H_ */
