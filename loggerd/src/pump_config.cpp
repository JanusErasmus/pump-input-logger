#include <string.h>
#include <stdlib.h>

#include "pump_config.h"

PumpConfig::PumpConfig()
{
    interval = -1;
    startHour = -1;
    stopHour = -1;
    runTime = -1;
    restTime = -1;

    mFD = fopen("/etc/loggerd/user.var", "r");
    if(!mFD)
    {
        printf("User variables could NOT be opened\n");
        return;
    }

    char line[64];
    while(fgets(line, 64, mFD))
        handleLine(line);
}

void PumpConfig::handleLine(char * line)
{
    char * p = strchr(line, '=');
    if(!p)
        return;

    char * arg = p + 1;
    *p = 0;
    //printf("%s -> %s\n", line, arg);

    if(!strncmp(line, "rate", 4))
    {
        interval = atoi(arg) * 60;
    }
    if(!strncmp(line, "start", 4))
    {
        startHour = atoi(arg);
    }
    if(!strncmp(line, "stop", 4))
    {
        stopHour = atoi(arg);
    }
    if(!strncmp(line, "on", 2))
    {
        runTime = atoi(arg) * 60;
    }
    if(!strncmp(line, "rest", 4))
    {
        restTime = atoi(arg) * 60;
    }
}

PumpConfig::~PumpConfig()
{
    if(mFD)
        fclose(mFD);
}

