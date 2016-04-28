#ifndef EVENT_H_
#define EVENT_H_
#include <Arduino.h>

class Event
{
	boolean isValid();

public:
	long timeStamp;  //4 bytes
	uint8_t port;	//1 byte
	uint8_t state;   //1 byte
	uint8_t ack;	//1 byte
	uint8_t crc;	//1 byte

	Event();
	Event(long timeStamp, uint8_t port, uint8_t state);
	Event(int address);

	void store(int address);
	void acknowledge();
	String getString();

	void print();

} __attribute__((packed));

#endif /* EVENT_H_ */
