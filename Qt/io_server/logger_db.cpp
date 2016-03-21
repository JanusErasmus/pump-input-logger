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

    emit lockCountChanged();

    qDebug() << "DB inserted event";

    mQueryMutex.unlock();

    return true;
}

bool LoggerDB::updateLock(QString loggerSerial)
{

    QDateTime local(QDateTime::currentDateTime());

    mQueryMutex.lock();

   /* //if the lock is not in the database, add it
    if(info->serial)
    {
        //if it was previously added, assign it this serial
        if(info->barcode.length())
        {
            if(serialExist(info->barcode))
            {
                qDebug() << "Set serial for " + info->barcode;
                QSqlQuery query;
                if(query.exec("UPDATE events SET serial= \"" + loggerSerial + "\" where barcode = \""  + info->barcode + "\""))
                {
                    while (query.next())
                    {
                        qDebug() << query.value(0);
                    }
                }
                else
                {

                    qDebug() << "possible duplicate entry";
                    QSqlQuery query;
                    if(query.exec("DELETE FROM locks WHERE serial_id= \"" + info->barcode + "\" AND barcode = \""  + info->barcode + "\""))
                        qDebug() << "Entry removed";
                }
            }
            else
                if(!serialExist(info->serial))
                    insertLock(info->serial);
        }
        else
        {
            if(!serialExist(info->serial))
                insertLock(info->serial);
        }
    }
    else if(info->barcode.length())
    {
        qDebug() << "No serial... Using barcode " << info->barcode;
        serialStr = info->barcode;

        if(!serialExist(info->barcode))
            insertLock(info->barcode);
        else
        {
            QSqlQuery query;
            query.exec("UPDATE locks SET serial_id= \"" + serialStr + "\" where barcode = \""  + info->barcode + "\"");
            while (query.next())
            {
                qDebug() << query.value(0);
            }
        }
    }
    else
    {
        qDebug() << "No serial or barcode";
        mQueryMutex.unlock();
        return false;
    }


    QSqlQuery query;
    bool status = query.exec("UPDATE locks SET baudrate= " + QString::number(info->baudRate) \
                             + ", vcap=" + QString::number(info->vCap) \
                             + ", vmotor=" + QString::number(info->vMotor) \
                             + ", vcpu=" + QString::number(info->vCPU) \
                             + ", flash=" + QString::number(info->flashProgramFlag) \
                             + ", vslope=" + QString::number(info->capSlopeFlag) \
                             + ", chargetime=" + QString::number(info->chargeTime) \
                             + ", baudrate=" + QString::number(info->baudRate) \
                             + ", pollsuccess=" + QString::number(info->pollSuccess) \
                             + ", opensuccess=" + QString::number(info->openSuccess) \
                             + ", openadc=" + QString::number(info->openADC) \
                             + ", closesuccess=" + QString::number(info->closeSuccess) \
                             + ", closeadc=" + QString::number(info->closeADC) \
                             + ", secure=" + QString::number(info->secureFlag) \
                             + ", manufacture_batch=" + QString::number(mBatch) \
                             + ", time_added= \"" + local.toString("yyyy-MM-dd HH:mm:ss") + "\"" \
                             + " where serial_id=\"" + serialStr + "\"");

    if(!status)
    {
        qDebug("Could not update database");
        mOpenOk = false;

        mQueryMutex.unlock();
        return false;
    }

    if(info->barcode.length())
    {
        query.exec("UPDATE locks SET barcode= \"" + info->barcode \
                    + "\" where serial_id=\"" + serialStr + "\"");


        if(error)
        {
            query.exec("UPDATE locks SET error_id= " + QString::number(error) \
                        + ", passed=\"0\" where serial_id=\"" + serialStr + "\"");
        }
        else
        {
            query.exec("UPDATE locks SET passed=\"1\" where serial_id=\"" + serialStr + "\"");
        }
    }

    if(info->hiTag.length())
    {
        query.exec("UPDATE locks SET hitag= \"" + info->hiTag \
                    + "\" where serial_id=\"" + serialStr + "\"");

    }
    */

    mQueryMutex.unlock();

    return true;
}


LoggerDB::~LoggerDB()
{
    close();

    qDebug("remove QMYSQL");
    db.removeDatabase("QMYSQL");
}


