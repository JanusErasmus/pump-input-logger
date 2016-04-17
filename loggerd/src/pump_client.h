#ifndef SRC_PUMP_CLIENT_H_
#define SRC_PUMP_CLIENT_H_
#include "pump_db.h"

class PumpClient
{
    PumpDB mDB;
    int mFD;
    char mMACstring[23];
    int mRSSI;

    int handleClient(char * buf);
    int parseArgs(char * string, int argc, char ** argv, char c1, char c2);
    bool handleLine(char * line);

    bool writeClient(char * buf);

public:
    PumpClient(int fd, struct sockaddr_in * client);
    virtual ~PumpClient();
};

#endif /* SRC_PUMP_CLIENT_H_ */
