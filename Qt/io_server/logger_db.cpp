#include <QDebug>
#include <QVariant>
#include <QDateTime>
#include <QtSql/QSqlQuery>

#include <stdlib.h>
#include <stdio.h>

#include "logger_db.h"

LoggerDB::LoggerDB(QString server, int port) : QWidget(0), mServer(server), mPort(port)
{    
    db = QSqlDatabase::addDatabase("QMYSQL");    
    mOpenOk = false;
}

bool LoggerDB::open()
{
    if(mServer.isEmpty())
        return 0;

    mOpenOk = false;
    qDebug() << "Opening MYSQL:" << mServer << ":" << mPort;

    db.setHostName(mServer);
    if(mPort)
        db.setPort(mPort);

    db.setDatabaseName("pumplog");
    db.setUserName("root");
    db.setPassword("Victor874");
    bool status = db.open();

    if(!status)
    {
        qDebug("Database could NOT be opened");
        return false;
    }

    mOpenOk = true;

    return true;
}

bool LoggerDB::close()
{
    if(db.isOpen())
    {
        qDebug("Closing DB");
        db.close();

         mOpenOk = false;
    }

    return true;
}


bool LoggerDB::insertEvent(QDateTime evtTime, int port, int state)
{
    mQueryMutex.lock();
    QSqlQuery query;
    bool status = query.exec("INSERT INTO events (time, port, state) values(\"" + evtTime.toString("yyyy-MM-dd HH:mm:ss") + "\"," + QString::number(port) + "," + QString::number(state) + ")");

    if(!status)
    {
        qDebug("Could not add to database");

        return false;
    }

    qDebug() << "DB inserted event";

    mQueryMutex.unlock();

    return true;
}

bool LoggerDB::updateLogger(QString loggerMAC , int rssi)
{

    mQueryMutex.lock();

    bool found = false;

    QSqlQuery query;
    bool status = query.exec("SELECT * FROM loggers WHERE mac = \"" + loggerMAC + "\"");

    if(status)
    {

        while (query.next())
        {
            if(!query.value(0).toString().isEmpty())
            {
                qDebug("Found logger");
                found = true;
            }
        }

    }


    if(found)
    {
        status = query.exec("UPDATE loggers SET last_logged = NOW(), rssi = " + QString::number(rssi) + " WHERE mac = \"" + loggerMAC + "\"");
        if(!status)
        {
            qDebug("Could not update database");
            return false;
        }
    }
    else
    {
        status = query.exec("INSERT into loggers (mac, rssi) values(\"" + loggerMAC + "\"," +  QString::number(rssi) + " )");
        if(!status)
        {
            qDebug("Could not insert into database");
            return false;
        }
    }

    mQueryMutex.unlock();

    return true;
}


LoggerDB::~LoggerDB()
{
    close();

    qDebug("remove QMYSQL");
    db.removeDatabase("QMYSQL");
}


