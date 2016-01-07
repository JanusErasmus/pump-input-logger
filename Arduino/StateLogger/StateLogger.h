#ifndef StateLogger_h
#define StateLogger_h

#include <arduino.h>

uint8_t calc_crc(uint8_t * buff, uint8_t len);

struct s_event //8 byte event
{
   long timeStamp;
   uint8_t port;
   uint8_t state;   
   uint8_t ack;
   uint8_t crc;

   s_event();
   s_event(long timeStamp, uint8_t port, uint8_t state);
   s_event(int address);
   
   void store(int address);
   void acknowledge();
   String getString();
   void print();

 private:
	boolean isValid();
	
} __attribute__((packed));

class StateLogger
{
  int mStartAddress;
  int mHeadAddress;
  int mTailAddress;
  int mCurrAddress;
  
public:
  StateLogger(int address);
  void log(s_event * evt);
  void reset(){ mCurrAddress = mHeadAddress; };
  s_event getEvent();
  void ack();
  void clear();

  void showAll();
};

#endif //StateLogger