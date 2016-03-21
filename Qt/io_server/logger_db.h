#ifndef LOCK_DB_H
#define LOCK_DB_H
#include <QWidget>
#include <QtSql/QSqlDatabase>
#include <QMutex>
#include <stdint.h>

class LoggerDB : public QWidget
{    
    Q_OBJECT

    QSqlDatabase db;

    bool mOpenOk;

    QMutex mQueryMutex;

    int mBatch;
    QString mServer;
    int mPort ;


public:
    LoggerDB(QString server, int port);
    ~LoggerDB();

    bool open();
    bool close();

    QString getServer(){ return mServer; }
    int getPort(){ return mPort; }

    bool isOpen(){ return mOpenOk; }

    bool insertEvent(QDateTime evtTime, int port, int state);
    bool updateLock(QString loggerSerial);



signals:
    void lockCountChanged();

};

#endif // LOCK_DB_H
