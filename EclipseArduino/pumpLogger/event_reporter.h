#ifndef EVENT_REPORTER_H_
#define EVENT_REPORTER_H_
#include <IPAddress.h>
#include <WiFiClient.h>

class EventReporterClass
{
public:
	enum eReportState
	{
		RP_UNKNOWN,
		RP_IDLE,
		RP_CONNECT,
		RP_CLIENT,
		RP_TRANSFER,
		RP_DISCONNECT
	};

private:
	bool mProbed;



	eReportState mState;

	int mRSSI;
	const char * mSSID;
	const char * mPassword;
	IPAddress mServer;
	WiFiClient mClient;

public:
	EventReporterClass(const char * ssid, const char * pass, IPAddress server);
	virtual ~EventReporterClass();

	void service();

	uint8_t status();
	void printStatus(int status = -1);
};

extern EventReporterClass EventReporter;

#endif /* EVENT_REPORTER_H_ */
