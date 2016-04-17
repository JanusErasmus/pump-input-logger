#include "pump_db.h"
#include <time.h>
#include <stdio.h>


PumpDB::PumpDB()
{
    mConnection = mysql_init(NULL);
}

bool PumpDB::connect(const char * host, const char * user, const char * password)
{
    if (mConnection == NULL)
    {
        fprintf(stderr, "%s\n", mysql_error(mConnection));

        return false;
    }

    if (mysql_real_connect(mConnection, host, user, password, "pumplog", 0, NULL, 0) == NULL)
    {
        fprintf(stderr, "%s\n", mysql_error(mConnection));
        mysql_close(mConnection);

        mConnection = 0;
        return false;
    }

    return true;

}

bool PumpDB::insertEvent(PumpEvent * event)
{
    if (mConnection == NULL)
        return false;

    char queryString[256];
    char timeString[64];
    struct tm * timeStamp = localtime((time_t*)&event->sampleTime);
    strftime(timeString, 64, "%F %R", timeStamp);

    snprintf(queryString, 256, "INSERT INTO events (time, port, state) values(\"%s\",%d,%d)", timeString, event->port, event->state);

    if (mysql_query(mConnection, queryString))
    {
        fprintf(stderr, "%s\n", mysql_error(mConnection));

        return false;
    }

    return true;
}

bool PumpDB::getEvent()
{
    if (mConnection == NULL)
            return false;

        if (mysql_query(mConnection, "INSERT INTO events (time, port, state) values(\"2016-04-16 15:00\",9,5)"))
        {
            fprintf(stderr, "%s\n", mysql_error(mConnection));

            return false;
        }


        MYSQL_RES * result = mysql_store_result(mConnection);
        if (result == NULL)
        {
            fprintf(stderr, "PumpDB: ERROR - %s\n", mysql_error(mConnection));

            return false;
        }

        int num_fields = mysql_num_fields(result);

        printf("Result %d\n", num_fields);

        MYSQL_ROW row = mysql_fetch_row(result);
        if(row[0])
            printf(row[0]);

        mysql_free_result(result);

        return true;
}

bool PumpDB::loggerLogin(char * mac, int rssi)
{
    if (mConnection == NULL)
        return false;

    char query[256];
    sprintf(query, "UPDATE loggers SET last_logged = NOW(), rssi = %d WHERE mac = \"%s\"", rssi, mac);

    if (mysql_query(mConnection, query))
    {
        fprintf(stderr, "%s\n", mysql_error(mConnection));

        return false;
    }

    return true;
}

PumpDB::~PumpDB()
{
    mysql_close(mConnection);
}

