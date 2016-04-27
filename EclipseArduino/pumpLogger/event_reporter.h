#ifndef EVENT_REPORTER_H_
#define EVENT_REPORTER_H_
#include <IPAddress.h>
#include <WiFiClient.h>

class EventReporter
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
	WiFiClient * mClient;

public:
	EventReporter(const char * ssid, const char * pass, IPAddress server);
	virtual ~EventReporter();

	void service();

	uint8_t status();
	void printStatus(int status = -1);
};

extern EventReporter Reporter;

#endif /* EVENT_REPORTER_H_ */
