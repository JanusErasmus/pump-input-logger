#ifndef REPORTER_H_
#define REPORTER_H_
#include <Arduino.h>
#include <WiFiClient.h>

#include "state_logger.h"

class Reporter
{
	WiFiClient * mClient;
	StateLoggerClass * mLogger;

	void replyLogs();
	bool serviceServerData();

	String getMACstring(byte mac[6]);
	String getStateString(int port);

public:
	Reporter(WiFiClient * client, StateLoggerClass * logger);
	virtual ~Reporter();

	bool transfer();
};

#endif /* REPORTER_H_ */
