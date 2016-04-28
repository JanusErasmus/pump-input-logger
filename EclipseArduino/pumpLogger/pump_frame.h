#ifndef PumpFrame_h
#define PumpFrame_h

#include <arduino.h>

class PumpFrameClass
{
public:
  long reportRate;  //4
  uint8_t startHour; //1
  uint8_t endHour;  //1
  long upTime;  //4
  long restTime; //4
  uint16_t port; //2
  uint8_t server[4]; //4
  uint8_t crc; //1
  
  PumpFrameClass();
  PumpFrameClass(int address);

  void store(int address);
  boolean equals(PumpFrameClass * frame);
  void print();

  void setIP(String ipString, int flashAddress);
  void setPort(String portString, int flashAddress);

  PumpFrameClass& operator=(PumpFrameClass &frame);
  
}__attribute__((packed));

#endif //PumpFrame
