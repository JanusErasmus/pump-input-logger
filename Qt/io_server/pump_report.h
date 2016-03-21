#ifndef PUMPREPORT_H
#define PUMPREPORT_H
#include <QFile>

class pumpReport
{
    bool mOpened;
    QFile *mFile;
    int mEntryCount;

    void createReport(QString filename);
    void openReport(QString filename);
    QString readLastLine();
    int entryCount();

public:
    pumpReport();
    ~pumpReport();

    bool addEntry(QDateTime time, int port, int state);
};

#endif // PUMPREPORT_H
