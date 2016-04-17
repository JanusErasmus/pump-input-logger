#ifndef PumpFrame_h
#define PumpFrame_h

#include <arduino.h>

class PumpFrame
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
  
  PumpFrame();
  PumpFrame(int address);

  void store(int address);
  boolean equals(PumpFrame * frame);
  void print();

  void setIP(String ipString, int flashAddress);
  void setPort(String portString, int flashAddress);

  PumpFrame& operator=(PumpFrame &frame);
  
}__attribute__((packed));

#endif //PumpFrame
