#ifndef SRC_PUMP_EVENT_H_
#define SRC_PUMP_EVENT_H_
#include <stdint.h>

class PumpEvent
{
public:
    uint32_t sampleTime;
    uint8_t port;
    bool state;

    PumpEvent();
    PumpEvent(uint32_t stamp, uint8_t port, bool state);
    virtual ~PumpEvent();

    void print();
};

#endif /* SRC_PUMP_EVENT_H_ */
