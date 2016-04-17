#ifndef SRC_PUMP_DB_H_
#define SRC_PUMP_DB_H_
#include <mysql/mysql.h>

#include "pump_event.h"

class PumpDB
{
    MYSQL * mConnection;

public:
    PumpDB();
    virtual ~PumpDB();

    bool isConnected(){ return mConnection; } ;
    bool connect(const char * host, const char * user, const char * password);
    bool insertEvent(PumpEvent * event);
    bool getEvent();
    bool loggerLogin(char * mac, int rssi);
};

#endif /* SRC_PUMP_DB_H_ */
