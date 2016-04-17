#ifndef SRC_PUMP_CONFIG_H_
#define SRC_PUMP_CONFIG_H_
#include <stdio.h>

class PumpConfig
{
    FILE * mFD;
    void handleLine(char * line);

public:
    int interval;
    int startHour;
    int stopHour;
    int runTime;
    int restTime;

    PumpConfig();
    virtual ~PumpConfig();


};

#endif /* SRC_PUMP_CONFIG_H_ */
