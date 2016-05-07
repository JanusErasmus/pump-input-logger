#ifndef EVENT_REPORTER_H_
#define EVENT_REPORTER_H_
#include <Arduino.h>
#include <WiFiClient.h>

#include "event_logger.h"

class EventReporter
{
	WiFiClient * mClient;
	EventLoggerClass * mLogger;

	void replyLogs();
	bool serviceServerData();

	String getMACstring(byte mac[6]);
	String getStateString(int port);

public:
	EventReporter(WiFiClient * client, EventLoggerClass * logger);
	virtual ~EventReporter();

	bool transfer();
};

#endif /* EVENT_REPORTER_H_ */
