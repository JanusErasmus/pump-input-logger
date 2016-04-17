#include "pump_event.h"
#include <time.h>
#include <stdio.h>

PumpEvent::PumpEvent()
{
    sampleTime = time(0);
    port = 0;
    state = false;
}

PumpEvent::PumpEvent(uint32_t stamp, uint8_t port, bool state) : sampleTime(stamp), port(port), state(state)
{
}

void PumpEvent::print()
{
    struct tm * timeStamp = localtime((const long int*)&sampleTime);
    char timeString[64];
    strftime(timeString, 64, "%F %R", timeStamp);
    printf("Event %s %d %d\n", timeString, port, state);
}

PumpEvent::~PumpEvent()
{
}

