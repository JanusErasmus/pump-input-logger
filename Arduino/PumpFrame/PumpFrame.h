#ifndef PumpFrame_h
#define PumpFrame_h

#include <arduino.h>

class PumpFrame
{
public:
  uint8_t startHour;
  uint8_t endHour;
  long upTime;
  long restTime;
  uint8_t crc;
  
  PumpFrame();
  PumpFrame(int address);

  void store(int address);
  boolean equals(PumpFrame * frame);
  void print();

  PumpFrame& operator=(PumpFrame &frame);
  
}__attribute__((packed));

#endif //PumpFrame