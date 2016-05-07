#ifndef PumpFrame_h
#define PumpFrame_h
#include <arduino.h>

#include "Time.h"

class PumpFrameClass
{
public:
  time_t reportRate;  //4
  time_t upTime;  //4
  time_t restTime; //4
  uint8_t startHour; //1
  uint8_t endHour;  //1
  uint16_t port; //2
  uint8_t server[4]; //4
  char ssid[32]; //32
  char password[31]; //31
  uint8_t crc; //1
  
  PumpFrameClass();
  PumpFrameClass(int address);

  void store(int address);
  boolean equals(PumpFrameClass * frame);
  void print();

  void setIP(String ipString, int flashAddress);
  void setPort(String portString, int flashAddress);
  void setId(String addrString, int flashAddress);
  void setPass(String passString, int flashAddress);

  PumpFrameClass& operator=(PumpFrameClass &frame);
  
}__attribute__((packed));

#endif //PumpFrame
