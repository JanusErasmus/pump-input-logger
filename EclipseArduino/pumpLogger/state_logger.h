#ifndef StateLogger_h
#define StateLogger_h
#include <arduino.h>

#include "event.h"

class StateLoggerClass
{
  uint16_t mStartAddress;
  uint16_t mHeadAddress;
  uint16_t mTailAddress;
  uint16_t mCurrAddress;
  
public:
  StateLoggerClass(int address);
  void log(Event * evt);
  void reset(){ mCurrAddress = mHeadAddress; };
  Event getEvent();
  void ack();
  void clear();

  void showAll();
};

#endif //StateLogger
